# Grimoire

A pendulum, a sigil, a talisman and a wand for the Flipper Zero: the one place
a spell from [rickseufert.com](https://rickseufert.com) can leave the browser.
Claimed from [rseufert.github.io#147](https://github.com/rseufert/rseufert.github.io/issues/147).

![Pendulum](pendulum.png) ![Sigil](sigil.png)

![Charging](charging.png) ![Talisman](talisman.png)

![Hours](hours.png)

Built so far: the planetary-hours clock, the Pendulum, the Sigil and the
Talisman. Left and Right turn the pages.

| Page | What it does | Site issue |
|---|---|---|
| **Pendulum** | Hold a question in mind and press OK. The bob wanders, then settles: to and fro is yes, side to side is no, round and round means ask again. The answer comes from the hardware random source. A tick sounds at each end of the swing. | |
| **Sigil** | Press OK and type a wish on the Flipper's keyboard. Spare's reduction strikes the vowels and every repeated letter; the rest are drawn from stroke skeletons, each turned to one of eight directions, scaled, sometimes flipped and set off centre by a generator seeded from the wish, and bound with a ring (four letters or more) or a bar. The site's `temple/sigil.js` does exactly this, so a wish gives the same sigil here as there (`tools/sigil_check.js` proves it). Hold OK for four seconds to charge it: the strokes shake, a tone climbs two octaves, the LED heats. At full charge it burns to ash and is gone. Nothing is stored. | [#141](https://github.com/rseufert/rseufert.github.io/issues/141) |
| **Talisman** | Agrippa's square of the current hour's planet (Up and Down pick another), with the sigil of its intelligence traced solid, of its spirit dotted, and, after holding OK to type one, a name of yours in a thick line by his Latin numeration. OK cycles the three. A ring marks the start of a trace and a bar the end; a number too big for the square is read with its last figure dropped until it fits, as the old sigils were made. The data and the rules are the site's `temple/talisman.js`. | [#143](https://github.com/rseufert/rseufert.github.io/issues/143) |
| **Wand** | Still to come: a Sub-GHz blessing with the planetary hour in the payload, and a receive mode so a second Flipper can answer it. | |
| **Hours** | Today's table of planetary hours, twelve per screen; Up and Down flip between the day and the night. The current hour is marked. | [#140](https://github.com/rseufert/rseufert.github.io/issues/140) |

## The hour

Every page's footer names the planetary hour: whose it is, which of the twelve
of the day or the night, and when it ends. From sunrise the hours belong to
Saturn, Jupiter, Mars, the Sun, Venus, Mercury and the Moon in rotation, the
day's first hour names the day, and day and night hours are each a twelfth of
the daylight or the dark. `hours.c` computes sunrise and sunset with NOAA's
formulas for the same place and constants as the site's `js/hours.js`
(Portland, with the United States daylight-saving rule); the Flipper's clock
is taken as local time.

## Checking the engines on a host

The three engines have no Flipper dependencies, and each has a check:

```
cc -Wall -o /tmp/hours_test tools/hours_test.c hours.c -lm && /tmp/hours_test
cc -Wall -o /tmp/talisman_test tools/talisman_test.c talisman.c hours.c -lm && /tmp/talisman_test
cc -Wall -o /tmp/sigil_test tools/sigil_test.c sigil.c -lm && /tmp/sigil_test "the wish"
node tools/sigil_check.js path/to/temple/sigil.js "the wish"
```

The first compares sunrise, sunset and hours with published Portland times
and the classic table; the second checks every square's rows, columns,
diagonals and contents and the names' numbers; the last two print a sigil's
strokes from the C and from the site's own JavaScript for the same wish, and
the outputs are identical to four decimals.

## Controls

- Left / Right: switch page
- OK: Pendulum: ask. Sigil: write a wish. Talisman: next trace
- Hold OK: Sigil: charge. Talisman: write a name
- Up / Down: Talisman: another planet. Hours: day or night
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
