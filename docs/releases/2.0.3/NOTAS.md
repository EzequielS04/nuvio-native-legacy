# Nuvio Legacy 2.0.3

New Settings, and a long list of fixes. DRAFT, waiting for the owner.

## Settings

- **Settings redesigned.** Categories by task (Watch, Screens, Account and system), plain-language labels, a written On/Off state on every switch, and the technical options behind "More options" in each category.
- **A preview scene for each option**, so you see what it changes before you change it.
- **Panel or List layout** for Settings (#339).
- **Search options** (#311): Nuvio catalog results first, last or off; hide the "from <source>" caption. The Cinemeta switch only shows when it can do something.
- **Next episode in Continue watching** can be turned off, and a single up-next card can be removed (it stays hidden for that profile and on Trakt).
- **TV guide:** choose which channel add-ons show up (#283).
- **Subtitle size up to 250%** (#335).
- **Support the project** shows the Patreon and Ko-fi QR codes again.
- The usage guide now covers 2.0.

## Fixed

- **Crashes from threads racing each other** (watched episodes, parallel source check, catalog episodes, sign-in token, collections) (#203, #323).
- **LG webOS 4:** startup crash from FreeType symbols, and startup crashes are now reported on the next launch (#317).
- **Android:** start hang on Shield and other boxes, sign-in through the system network stack when libcurl fails (#266, #332); MStar flicker fixes and a crash report on Android 11 and older (#318).
- **Home:** switching profile no longer shows the previous profile's rows (#294); the hero no longer depends on a drawn row (#327); rebuilds are spaced out so Home settles with many collections (#319, #280).
- **Add-ons with very long URLs** (2 to 16 KB) are no longer ignored, and accounts with more than 32 add-ons are no longer cut silently.
- **Auto-play** never picks an add-on placeholder; Samsung never picks Dolby Vision (#284).
- **Embedded subtitles:** large MKV track headers, gentler reads, no stale downloads (#308, #330); more add-on subtitles (#268); Arabic subtitles with real bold and sharp ASS in 4K (#335).
- **Duplicate episodes** in the episode list (#328).
- **LG:** resume no longer drops the source on a failed seek (#246); DTS on webOS 26 (#285).
- **Samsung:** Pause and subtitle list errors (#269); aspect/zoom button on Tizen 4/5.
- **Player:** UP at the top of the controls hides them (#305); holding fast-forward speeds up and crosses the file in about 10 s (#340); skip intro and next episode use markers checked against the episode, with a timed fallback.
- **Trakt/Simkl sign-in** no longer freezes the screen while sending credentials.
- **Speed test** measures sources served as octet-stream.
- Saved: section label follows the open card. Glass outline follows the poster radius. Search translated on the spot, "All sources" in Sync (#312).
- The log is sent once per session, at most 64 KB.

## Notes

| Platform | File |
| --- | --- |
| LG webOS 3+ | `space.nuvio.native.legacy_2.0.3_arm.ipk` |
| LG with more RAM | `space.nuvio.native.legacy_2.0.3_arm-highcache.ipk` |
| Samsung Tizen 4 / 5 / 5.5 | `Nuvio-2.0.3-NuvioTpk40.tpk` |
| Samsung Tizen 6 | `Nuvio-2.0.3-NuvioTpk60.tpk` |
| Samsung Tizen 6.5 / 7 | `Nuvio-2.0.3-NuvioTpk65.tpk` |
| Samsung Tizen 8 / 9 | `Nuvio-2.0.3-NuvioTpk.tpk` |
| Samsung web app, Tizen 5.5+ | `NuvioTV-2.0.3-tizen.wgt` |
| Android TV / Google TV, Android 7+ | `Nuvio-2.0.3-android.apk` |
