# Phase 1599: original mutant schedule catalog and native image

Reopened original private server.dll; SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Extracted all 19 class-specific schedule source records and matched each registration ID to its adjacent parser-vector call, not registration index. Shared task/condition IDs authenticated by original push pairs in 220acb9b..220ad990 and 2205a7d7..2205ad76; shared SCHED_IDLE_WANDER=3 at 2205db4b, ACT_IDLE=1, ACT_GLIDE=27, ACT_LAND=32; STATE:IDLE=1 in 2209e41b..2209e435. Named private activity IDs resolved through live dictionary, not guessed constants. Original parser insertion order retained (Minor 80,91,81,82,83,84,90,85,86,87,88,89). Custom Major task139 and conditions66..69; Minor tasks139..143 and conditions66..68 verified in registration.

|Kind|Local|Source VA|Parser call|Tasks|Conditions|
|---|---:|---|---|---:|---:|
|MAJOR_CHASE_ENEMY|80|224a0980|2237a759|6|12|
|MAJOR_MOVE_TO_AMBUSH|81|224a0828|2237a797|7|3|
|MAJOR_WAIT_AMBUSH|82|224a0788|2237a7d5|1|3|
|MAJOR_FRENZY|83|224a06e0|2237a813|3|0|
|MAJOR_IDLE_STAND|84|224a0570|2237a851|3|12|
|MAJOR_ALERT_STAND|85|224a03c0|2237a88f|4|13|
|MAJOR_PHYSICS_TOSS|86|224a0208|2237a8cd|8|4|
|MINOR_LEAP_ATTACK_ENEMY|80|224a2620|2237e206|7|2|
|MINOR_SIDESTEP|91|224a2598|2237e244|1|0|
|MINOR_LAND_RECOVER|81|224a24a0|2237e282|5|0|
|MINOR_POUNCE_OFF|82|224a2368|2237e2c0|5|1|
|MINOR_WAKE_ANGRY_NO_DISPLAY|83|224a2278|2237e2fe|5|0|
|MINOR_STAND_AND_SPIT|84|224a2188|2237e33c|4|3|
|MINOR_TRYING_TO_MORPH_UP|90|224a20c8|2237e37a|4|0|
|MINOR_BACK_UP_AND_SPIT|85|224a1fa0|2237e3b8|5|3|
|MINOR_HOP_RANDOMLY|86|224a1f18|2237e3f6|2|0|
|MINOR_HARASS_ENEMY|87|224a1e10|2237e434|6|1|
|MINOR_FALL_TO_GROUND|88|224a1d78|2237e472|2|0|
|MINOR_ATTACK_BARREL|89|224a1cd0|2237e4b0|3|0|

Major 32 tasks/47 condition references; Minor 49 tasks/10 conditions. Existing native namespace and schedule-state APIs produce original-shaped descriptors; no inherited task semantics copied. Transactional image construction, generation validation, nonoverlapping token interval, external fallback, exact task arguments and interrupt masks. Loaded test walks every task via existing native state cursor, checks each condition bit, dynamic private activities, missing activity and generation failure. The test is descriptor/cursor verification, NOT execution of every task. Original owner engine scheduler remains needed.

Original region VA / bytes / SHA256:
2237a5c5/875/fbb8630951752b91e8cfbb1e26af05607582da68b45cb0604c1bd872ae4cf906
2237df80/1333/a5e167fa536fceb87f2db45b60e33b481e9a9bd6d35584b5280b135c8cc104b4
220acb9b/3573/21f9f22285f93a13bacf96a8ece7c7c408c60483bd60b6d5926b5db70e52f883
2205a7d7/1439/de5d633556a60e3287cf7a38d0cc5c9bf68fae4e70205ee4118e45f13cbdedab
2205db41/27/d425c7f4c271121bd5f3723d85b4028b171d436416406b7bdcdd039da5855439
224a0208/2428/d319ab4889e79bb140fed5b682f051ff044e85a331abfb64fe4b21363f3acadc
224a1cd0/2732/b5c34c401b77483c428bba82b909c04e830dad56a1eab4be8fae9fc85a610d7e

No original executable bytes committed. CI pending.
