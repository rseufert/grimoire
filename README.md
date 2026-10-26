# Grimoire

A pendulum, a sigil, a talisman and a wand for the Flipper Zero: the one place
a spell from [rickseufert.com](https://rickseufert.com) can leave the browser.
Claimed from [rseufert.github.io#147](https://github.com/rseufert/rseufert.github.io/issues/147).

Built so far: the planetary-hours clock and the Pendulum. The plan, in order:

| Page | What it does | Site issue |
|---|---|---|
| **Pendulum** (done) | Hold a question in mind and press OK. The bob wanders, then settles: to and fro is yes, side to side is no, round and round means ask again. The answer comes from the hardware random source. A tick sounds at each end of the swing. | |
| **Sigil** | Type a wish on the Flipper's keyboard. Spare's reduction drops the vowels and every repeated letter; a glyph is composed from the remaining letters' strokes, seeded from the sentence so the same wish gives the same sigil as the site's engine. Hold OK to charge it; it burns off the screen and is never stored. | [#141](https://github.com/rseufert/rseufert.github.io/issues/141) |
| **Talisman** | The planet of the current hour, computed on the device from the clock and Portland's sunrise by the Chaldean rule, its magic square from Agrippa with the spirit's sigil traced across it, and your name joined across the cells. | [#143](https://github.com/rseufert/rseufert.github.io/issues/143), [#140](https://github.com/rseufert/rseufert.github.io/issues/140) |
| **Wand** | A Sub-GHz blessing with the planetary hour in the payload, and a receive mode so a second Flipper can answer it. | |

## The hour

Every page's footer names the planetary hour: whose it is, which of the twelve
of the day or the night, and when it ends. Hold OK for today's table, Up or
Down to flip between the day and the night. From sunrise the hours belong to
Saturn, Jupiter, Mars, the Sun, Venus, Mercury and the Moon in rotation, the
day's first hour names the day, and day and night hours are each a twelfth of
the daylight or the dark. `hours.c` computes sunrise and sunset with NOAA's
formulas for a fixed place, Portland, with the United States daylight-saving
rule applied; the Flipper's clock is taken as local time. The module has no
Flipper dependencies; `tools/hours_test.c` checks it on a host:

```
cc -Wall -o /tmp/hours_test tools/hours_test.c hours.c -lm && /tmp/hours_test
```

## Controls

- Left / Right: switch page
- OK: Pendulum: ask
- Hold OK: today's hours; Up / Down there for day or night; Back or OK to close
- Back: exit

## Building

Needs [ufbt](https://github.com/flipperdevices/flipperzero-ufbt). Run `ufbt` in
this directory and the app lands in `dist/`. `ufbt launch` installs it on a
connected Flipper. `tools/assets.py` regenerates the icon.

## Sources

Austin Osman Spare, *The Book of Pleasure* (1913). Agrippa, *Three Books of
Occult Philosophy* II.22 and II.34. Both public domain.

## License

MIT. See [LICENSE](LICENSE).
