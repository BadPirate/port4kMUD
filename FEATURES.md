# Port4k MUD Features

This MUD includes all the features of CircleMUD 3.0 bpl 11 (baseline commit `170ce8a`), plus the following additions and modifications developed over its history. This list is based on a comprehensive review of all source files (`.c`, `.h`) in the `src/` directory changed since the baseline commit.

## Character Options

*   **Races:** Beyond Human, Elf, Dwarf, Hobbit, playable races include:
    *   Wolf (+1 Dex, +1 Cha, -1 Con, -1 Str)
    *   Giant (+3 Str, -2 Int, -1 Wis)
    *   Gnome (+2 Int, -1 Wis, -1 Dex)
    *   Pixie (+2 Dex, -2 Str)
    *   Gargoyle (+2 Con, +1 Str, -2 Int, -1 Cha)
    *   Brownie (+1 Int, +1 Dex, -2 Cha)
    *   Troll (+2 Str, -2 Cha)
    *   *Note: Each race has specific stat modifiers (`races.c`) and potential item restrictions (`class.c`).*
*   **Classes:** Beyond Magic User, Cleric, Thief, Warrior, playable classes include:
    *   Paladin (Hybrid, likely Str/Cha/Wis focused)
    *   Vampyre (Hybrid, likely Int/Dex/Cha focused, spellcaster)
    *   Bard (Hybrid, likely Cha/Int/Dex focused, spellcaster)
    *   Scout (Hybrid, likely Dex/Str focused, skill-based)
    *   Ninja (Advanced/Remort class? High skill caps)
    *   Master (Advanced/Remort class? High skill caps)
    *   *Note: Each class has unique stat priorities, THAC0 progression, skill/spell learning parameters (`class.c`), and guild locations (`spec_assign.c`, `class.c`).*
*   **Remorting:** A system allowing players to restart their character progression, potentially gaining permanent advantages (`spells.c`, `structs.h`).
    *   `remort`: Initiate the remorting process (may use `questpoints`).
*   **Quest Points:** A value tracked per player (`structs.h`), likely earned through quests or other means, potentially used for remorting or by the Quest Token system.
*   **Player Killing (PK):**
    *   `pkill` toggle: Allows players to opt-in/out of PK (`structs.h`).
    *   Detailed PK stats tracked: `pklevel`, `pkdeaths`, `pkills` (`structs.h`).

## Major Systems

*   **Online Creation (Oasis OLC):** A full suite of in-game building tools (`olc.c`, `*.edit.c`, `olc.h`).
    *   `redit`, `oedit`, `medit`, `sedit`, `zedit`, `tedit` (text file editor).
    *   `olc`, `olist`, `mlist`, `rlist`.
*   **Clan System:** Allows players to form and manage clans (`clan.c`, `clan.h`, `structs.h`, `db.c`).
    *   `clan`: Manage membership, ranks, etc.
    *   `clansay`: Clan communication channel.
    *   Clan data saved separately.
*   **Player Housing:** Players can buy, sell, and manage personal houses (`house.c`, `house.h`, `structs.h`, `db.c`).
    *   `house`: Player command for interaction.
    *   `hcontrol`: Immortal command for management.
    *   House data saved separately.
*   **Auction House:** A global system for players to auction items (`auction.c`, `auction.h`, `db.c`).
    *   `auction`, `bid`, `aucsay`/`asay`, `aucid`.
*   **Arena:** A dedicated area for Player vs. Player combat (`arena.c`, `spec_procs.c`).
    *   `arena`: Command to enter/manage participation.
    *   `asay`: Arena communication channel.
    *   Managed by `arena_sentinel` special procedures.
*   **Mail System:** Allows players to send messages and items offline (`mail.c`, `mail.h`, `db.c`).
    *   `mail`: Command to read/send mail (uses spec_proc or dedicated functions).
*   **Bulletin Boards:** In-game forums (`boards.c`, `boards.h`, `db.c`).
    *   `write`: Post messages.
*   **Gambling:** Games of chance via special procedures (`spec_procs.c`: `gamble_master`, `jerry`).
    *   `gamble`: Initiate gambling (requires specific NPC/location).
    *   Options: Bet money (capped), bet high cost for Quest Tokens, bet items for chance at better items.
*   **Quest Token System:** NPC (`quest_token_master` in `spec_procs.c`) exchanges tokens for rewards.
    *   Rewards: Item stat improvement, passes, permanent stat/pool increases, hunger/thirst cure, money.
*   **Corpse Retrieval:** NPC (`corpse_guy` in `spec_procs.c`) retrieves player corpses for a fee.
    *   `corpse buy`, `corpse back`, `corpse price`.
*   **Auto-Save/Crash Recovery:** Replaces traditional rent. Player inventory/equipment saved automatically on disconnect/crash (`objsave.c`).
    *   Admin utility `listrent` to view saved inventories.

## Utility & Quality of Life

*   **Copyover:** Server reboots with minimal disruption (`comm.c`).
    *   `copyover`: (Immortal) Initiates the process.
*   **MCCP Support:** Mud Client Compression Protocol implemented (`comm.c`).
*   **Command Aliases:** Player-defined command shortcuts (`alias.c`, `structs.h`).
    *   `alias`: Define or list aliases.
*   **ANSI Color & Customization:** Enhanced text display (`color.c`, `structs.h`).
    *   `color`: Toggle color on/off or customize.
*   **Screen Display Customization:** Set screen width and page length (`structs.h`, likely `display` command).
*   **Auto Toggles:** Numerous settings players can toggle (`structs.h`, various `act.*.c` files).
    *   `autoexit`, `autosplit`, `autoloot`, `autogold`, `autoassist`, `autowimp`.
    *   `brief`, `compact`, `norepeat`, `noouch`.
    *   Channel toggles: `nogossip`, `noauction`, `nomusic`, `nograts`, `noshout`, `notell`, `nowiz`.
*   **Enhanced Communication:** (`act.comm.c`)
    *   Channels: `gossip`, `grats`, `music`, `ouch`, `newbie`.
    *   `quote`: Repeat last channel message.
    *   `reply`: Reply to last tell.
    *   Standard: `say`, `tell`, `ask`, `gsay`, `shout`, `holler`.
*   **Player Info:** (`act.informative.c`)
    *   `scan`: View adjacent rooms/occupants.
    *   `prompt`: Customize prompt display.
    *   `who`, `users`, `last` (shows last logon time/host from `structs.h`).
    *   `time`, `weather`, `where`, `consider`, `examine`.
    *   `gold`, `inventory`, `equipment`, `score`, `skills`, `affects`.
*   **Immortal Utilities:** (`act.wizard.c`, `util/`)
    *   Standard: `force`, `load`, `purge`, `restore`, `return`, `set`, `shutdown`, `reboot`, `stat`, `switch`, `syslog`, `teleport`, `transfer`, `goto`, `wiznet`, `wizlock`, `ban`, `unban`, `freeze`, `pardon`, `mute`, `squelch`, `vnum`, `vstat`, `zreset`.
    *   Added: `leaks` (memory leak check), `rclone` (room clone), `pclean` (purge inactive players via `util/purgeplay`), `flush` (save player files).
    *   Offline Utils: `autowiz`, `listrent`, `mudpasswd`.

## Combat & Skills

*   **Core Combat:** `kill`, `hit`, `flee`, `assist`, `bash`, `kick`, `rescue`, `backstab` (`act.offensive.c`).
*   **Skills/Spells:** (`spells.c`, `magic.c`, `spell_parser.c`, `constants.c`)
    *   `cast`, `practice`.
    *   Crafting: `brew`, `scribe`.
    *   Thief Skills: `hide`, `sneak`, `steal`, `pick`.
    *   Other Skills: `track` (`graph.c`), `howl`, `tame`, `bite` (`act.offensive.c`).
    *   `sacrifice`: Sacrifice item for gold.
    *   `combine`: Combine items (mechanics unclear).
    *   `statue`: Spell to turn victim into a statue.
*   **Mounts:** (`spells.c`, `act.movement.c`, `act.offensive.c`, `fight.c`, `structs.h`)
    *   `mount`, `dismount`.
    *   `buck`: Mount tries to throw rider.
    *   Mounts affect movement and combat.
*   **Tamed Mob Assistance:** Tamed mobs (`AFF_TAMED`) assist owner in combat (`fight.c`).

## World & Interaction

*   **Movement:** Standard directions plus diagonals (`act.movement.c`), `enter`, `leave`.
*   **World Interaction:** `get`, `drop`, `put`, `give`, `drink`, `eat`, `pour`, `wear`, `wield`, `grab`, `remove`, `use`, `open`, `close`, `lock`, `unlock` (`act.item.c`, `act.other.c`).
*   **Extensive Socials:** Vast array of socials using `do_action` (`act.social.c`).
*   **Character:** `save`, `quit`, `title`, `prompt`, `display`, `wimpy` (`act.other.c`).
*   **Grouping:** `group`, `ungroup`, `follow`, `report` (`act.other.c`).
*   **Unique Zone Content (King's Castle - Zone 150):** Numerous NPCs with unique behaviors via special procedures (`castle.c`, `spec_assign.c`).
*   **Object Sentinels:** NPCs blocking passage without specific items (`spec_procs.c`: `obj_sentinel`).
*   **Minor NPC Behaviors:** Item-stealing monkeys, auto-selling dump rooms, healing clerics, etc. (`spec_procs.c`).

## Disabled/Replaced Standard Features

*   **Standard Shops:** Commands (`buy`, `sell`, etc.) disabled (`interpreter.c`); handled by OLC (`sedit`) and shopkeeper spec_procs (`shop.c` likely unused/minimal).
*   **Standard Rent:** Commands (`rent`, `offer`) disabled (`interpreter.c`); replaced by Auto-Save/Crash Recovery system (`objsave.c`).
*   **Advance Command:** Disabled (`interpreter.c`); level gain likely automatic or via trainers (spec_proc).
*   **Identify Command:** Disabled (`interpreter.c`); likely handled by `examine` or spells/skills.