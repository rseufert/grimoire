# Grimoire

A pendulum, a sigil, a talisman and a wand for the Flipper Zero: the one place
a spell from [rickseufert.com](https://rickseufert.com) can leave the browser.
Claimed from [rseufert.github.io#147](https://github.com/rseufert/rseufert.github.io/issues/147).

Nothing is built yet beyond a title page. The plan, in order:

| Page | What it will do | Site issue |
|---|---|---|
| **Pendulum** | A dowsing answer from the hardware random source, drawn swinging on the screen. Yes, no, ask again. | |
| **Sigil** | Type a wish on the Flipper's keyboard. Spare's reduction drops the vowels and every repeated letter; a glyph is composed from the remaining letters' strokes, seeded from the sentence so the same wish gives the same sigil as the site's engine. Hold OK to charge it; it burns off the screen and is never stored. | [#141](https://github.com/rseufert/rseufert.github.io/issues/141) |
| **Talisman** | The planet of the current hour, computed on the device from the clock and Portland's sunrise by the Chaldean rule, its magic square from Agrippa with the spirit's sigil traced across it, and your name joined across the cells. | [#143](https://github.com/rseufert/rseufert.github.io/issues/143), [#140](https://github.com/rseufert/rseufert.github.io/issues/140) |
| **Wand** | A Sub-GHz blessing with the planetary hour in the payload, and a receive mode so a second Flipper can answer it. | |

## Building

Needs [ufbt](https://github.com/flipperdevices/flipperzero-ufbt). Run `ufbt` in
this directory and the app lands in `dist/`. `ufbt launch` installs it on a
connected Flipper. `tools/assets.py` regenerates the icon.

## Sources

Austin Osman Spare, *The Book of Pleasure* (1913). Agrippa, *Three Books of
Occult Philosophy* II.22 and II.34. Both public domain.

## License

MIT. See [LICENSE](LICENSE).
