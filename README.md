# RPGServ
RPGServ for Anope.

# Installation Process

You want to drop the `rpgserv` folder into `modules` for Anope. Or simply extract the contents of `rpgserv` into `modules` instead of plopping the folder into `modules`.

Either way, it works just the same.

# RPGServ Configuration

To have RPGServ work, you need to have an `rpgserv.conf` in the `conf` folder for Services so that it can automatically come online with Services when it's started.

A sample `rpgserv.conf` has been provided. Adapt as you see fit.

## General Usage for RPGServ
Once `RPGServ` is loaded into `Anope`, the following commands and how to use it are made available in `irc`. Output was formatted by `mIRC`.

`/msg rpgserv help` outputs the following:

```
[04:14:17AM] -RPGServ- RPGServ commands:
[04:14:17AM] -RPGServ-     ACT            Does something
[04:14:17AM] -RPGServ-     CALC           Calculates a value, even when using random dice.
[04:14:17AM] -RPGServ-     D20            Rolls a d20 die against a difficilty.
[04:14:17AM] -RPGServ-     EXALTED        Rolls dice using the Exalted dice system.
[04:14:17AM] -RPGServ-     HELP           Displays this list and give information about commands
[04:14:17AM] -RPGServ-     ROLL           Calculates the resulting dice value, even when using advanced math functions.
[04:14:17AM] -RPGServ-     ROLL+          Rolls a specifed set of dice.
[04:14:17AM] -RPGServ-     SAY            Says something
[04:14:17AM] -RPGServ-     SHADOWRUN      Rolls dice using the Shadowrun dice system.
[04:14:17AM] -RPGServ-     WOD            Rolls dice using the Old World of Darkness dice system.
[04:14:17AM] -RPGServ-     WW             Rolls dice using the Newer World of Darkness dice system.
```

`/msg rpgserv help act`

```
[04:17:15AM] -RPGServ- Syntax: **act** where <what was done>
[04:17:15AM] -RPGServ-  
[04:17:15AM] -RPGServ- Performs an action in channel
[04:17:15AM] -RPGServ- Note: This command is ALWAYS logged.
[04:17:15AM] -RPGServ-
```
So, for example, if you wanted `RPGServ` to act out something for you in say, `#tavern`, you could do so via `/msg rpgserv act #tavern welcomes a guard into the tavern with a smile` and you'll get an output in `#tavern` of `RPGServ` doing its thing, similar to the below:

[04:28:16AM] * <RPGServ> welcomes a guard into the tavern with a smile

# LICENSE

Unknown. Was written by Janus on MagicStar Network.
