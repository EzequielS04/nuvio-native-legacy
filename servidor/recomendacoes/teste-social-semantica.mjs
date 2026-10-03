// Focused route tests against real SQLite, without Wrangler, network or a
// shared D1 database. Requires Node 22+; run:
//   node --test servidor/recomendacoes/teste-social-semantica.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { DatabaseSync } from "node:sqlite";
import test from "node:test";
import { rotaAmigo, rotaEvento } from "./src/social.js";

const NOW = 1791054000;
const ME = "nuvio:reader", FRIEND = "nuvio:friend";
const h = {
  agora: () => NOW,
  limpar: (s, max) => String(s ?? "").slice(0, max),
  json: (body, status = 200) => new Response(JSON.stringify(body), { status }),
  erro: (message, status) => new Response(message, { status }),
};

function fixture(t) {
  const sqlite = new DatabaseSync(":memory:");
  t.after(() => sqlite.close());
  for (const name of ["schema.sql", "migracao-005-amigos.sql", "migracao-006-social.sql"])
    sqlite.exec(readFileSync(new URL(name, import.meta.url), "utf8"));
  sqlite.prepare("INSERT INTO pessoa (id, nome, criado, visto, alcance) VALUES (?, ?, ?, ?, 1)")
    .run(ME, "Reader", NOW, NOW);
  sqlite.prepare("INSERT INTO pessoa (id, nome, criado, visto, alcance) VALUES (?, ?, ?, ?, 1)")
    .run(FRIEND, "Friend", NOW, NOW);
  sqlite.prepare("INSERT INTO contato (a, b, criado, via) VALUES (?, ?, ?, 'codigo')")
    .run(ME, FRIEND, NOW - 86400);

  // D1's prepared-statement API, executing the production SQL in SQLite.
  const DB = {
    prepare(sql) {
      const statement = sqlite.prepare(sql);
      return { bind(...values) {
        return {
          first: async () => statement.get(...values) || null,
          all: async () => ({ results: statement.all(...values) }),
          run: async () => statement.run(...values),
        };
      } };
    },
    batch: async (commands) => Promise.all(commands.map((command) => command.run())),
  };
  const env = { DB };
  const friend = async () => {
    const response = await rotaAmigo(env, { id: ME }, new URL(
      `https://example.test/v1/amigo?id=${FRIEND}`), h, async () => null);
    assert.equal(response.status, 200);
    return response.json();
  };
  const event = async (body) => {
    const response = await rotaEvento(env, { id: FRIEND }, body, h, async () => true);
    assert.equal(response.status, 200);
    return response.json();
  };
  return { sqlite, friend, event };
}

test("recommendation completion remains independent of reactions and opening", async (t) => {
  const { sqlite, friend, event } = fixture(t);
  const id = Number(sqlite.prepare(
    "INSERT INTO rec (de, para, criado, imdb, tipo, titulo) VALUES (?, ?, ?, 'tt1', 'movie', 'Movie')"
  ).run(ME, FRIEND, NOW).lastInsertRowid);
  const rec = async () => (await friend()).recs[0];
  assert.equal((await rec()).terminou, 0, "delivery is not completion");
  sqlite.prepare("UPDATE rec SET visto = 1, aberto = 1 WHERE id = ?").run(id);
  assert.equal((await rec()).terminou, 0, "opening is not completion");
  await event({ ev: "inicio", imdb: "tt1", rec: id });
  assert.equal((await rec()).terminou, 0, "starting is not completion");
  await event({ ev: "reacao", imdb: "tt1", rec: id, reacao: 1 });
  assert.equal((await rec()).estado, "reacao");
  assert.equal((await rec()).terminou, 0, "a positive reaction alone is not completion");
  await event({ ev: "fim", imdb: "tt1", rec: id, pct: 100 });
  const completed = await rec();
  assert.equal(completed.estado, "reacao", "the existing reaction display is preserved");
  assert.equal(completed.reacao, 1);
  assert.equal(completed.terminou, 1, "a reaction does not hide known completion");
});

test("Recently liked uses the latest reaction and retains the episode identity", async (t) => {
  const { friend, event } = fixture(t);
  const reaction = (imdb, reacao, extra = {}) => event({ ev: "reacao", imdb, reacao, ...extra });
  await reaction("tt1", 1);
  await reaction("tt1", -1);
  await reaction("tt2", 1);
  await reaction("tt2", 0);
  await reaction("tt3", 1);
  await reaction("tt3", -1, { midia: "series", temporada: 1, episodio: 1 });
  await reaction("tt4", 1, { midia: "series", temporada: 1, episodio: 1 });
  await reaction("tt4", 1, { midia: "series", temporada: 1, episodio: 2 });
  await reaction("tt4", -1, { midia: "series", temporada: 1, episodio: 1 });
  const liked = (await friend()).gostou;
  assert.ok(!liked.some((x) => x.imdb === "tt1"), "a newer dislike removes an older like");
  assert.ok(!liked.some((x) => x.imdb === "tt2"), "a newer neutral reaction removes an older like");
  assert.deepEqual(liked.map((x) => [x.imdb, x.midia, x.temporada, x.episodio]), [
    ["tt4", "series", 1, 2], ["tt3", "movie", 0, 0],
  ], "episodes and media kinds cannot overwrite one another's reactions");
});

test("reaction dedupe preserves a changed mind inside the retry window", async (t) => {
  const { sqlite, friend, event } = fixture(t);
  const body = { ev: "reacao", imdb: "tt1", midia: "series", temporada: 1, episodio: 3 };
  assert.equal((await event({ ...body, reacao: 1 })).evento, 1);
  assert.equal((await event({ ...body, reacao: 1 })).evento, 0, "the same latest reaction is a retry");
  assert.equal((await event({ ...body, reacao: -1 })).evento, 1);
  assert.equal((await event({ ...body, reacao: 1 })).evento, 1, "liking again is a new choice");
  assert.equal((await event({ ...body, reacao: 1 })).evento, 0);
  assert.equal(sqlite.prepare("SELECT COUNT(*) AS n FROM evento").get().n, 3);
  assert.equal((await friend()).gostou.length, 1);
  assert.equal((await event({ ...body, midia: "movie", reacao: 1 })).evento, 1,
    "a different media kind is not a retry of the episode");
});

test("private friend activity stays hidden after the reaction query changes", async (t) => {
  const { sqlite, friend, event } = fixture(t);
  await event({ ev: "reacao", imdb: "tt1", reacao: 1 });
  sqlite.prepare("UPDATE pessoa SET alcance = 0 WHERE id = ?").run(FRIEND);
  const result = await friend();
  assert.equal(result.compartilha, 0);
  assert.deepEqual(result.gostou, []);
  assert.equal(result.gosto, null);
});
