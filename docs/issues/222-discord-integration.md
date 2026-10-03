# Discord presence integration

The existing #222 feature is integrated into the current Settings layout, under Accounts and services. AJ_DISCORD is appended after the existing options so positional defaults and persisted option indices remain stable. Discord and Seekr build definitions are both retained across Android, ARM, and TPK configuration paths; the Samsung browser build keeps its WebSocket library.

The three new Settings strings have English translations. Portuguese remains Portuguese; other language tables currently use explicit English fallback for these new entries. Existing translations are unchanged.

Checks: Settings data/catalog and interaction fixtures, complete language-table alignment/order/format tests, Android and TPK Settings syntax dispatch, shell syntax, and configured build-macro generation without printing values. These checks do not prove OAuth authorization, account presence, browser OAuth CORS, or physical-TV operation. Runtime networking and concurrency must pass their separate review and tests before release.
