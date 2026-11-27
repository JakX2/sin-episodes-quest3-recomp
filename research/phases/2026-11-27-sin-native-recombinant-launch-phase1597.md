# Phase 1597: original Minor launch and mutant fail selection

Original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Start140 2237cca9: ground handle +238 null -> TaskComplete; otherwise ground+218 flags4 invokes 22228160(actor,ground,1,.5). Draw x/y signed ints [-1,1] repeatedly until either nonzero, normalize via imported helper, clear ground via 22221890(0), probe upward via 2237c8c0. If trace fraction ==1, relocate actor +360 by (0,0,1) via 2237c260. Both branches initialize vector(0,0,200) unconditionally at 2237cdb8 before adding normalized direction*200, then call velocity setter 22141a00. No arbitrary uninitialized vector in false path. Owner trace includes original +214 collision token captured before dirty origin refresh, start/end +2bc and mask 0x202400b, upward delta+1. Finite trace fraction required, RNG bounded to 64 pairs as explicit native safety guard. All actual engine physics remains owner-owned.

Major SelectFailSchedule 22377740 clears +c68, tail-jumps inherited 2204e6d0. Minor 2237d2a0 checks schedule85 or schedule23/task100, current10 ->41. Otherwise for those schedules and72/73 uses 2237c190: requires grounded byte +bc&1; downward trace -12 with same mask, fraction!=1 -> supported, delegates; no support ->86. Other schedules delegate. Original +424 Minor 2237b050 is RET, no new preschedule code needed.

Separate launch export preserves phase1596 API size and its explicit unrecovered Start140 route. A future dispatch composition must call start140 before older MinorTask.execute, rather than silently pretending older export implements it. Native loaded tests exercise both upward trace branches, RNG retry, ground-effect order and fail selection. No original executable bytes committed.

VA/bytes/SHA256:
2237cca9/323/7e573cd262f15dda25aa32720dfcd70b3de64d46f44ed5f920ef6da2f8a250d9
2237c8c0/178/b5cbc0dd4df2a3e2b3cde11deec09815b8451e9371be6e877a7982cf170c3d16
2237c260/88/11c831ffca3fb1325461bd80d815c133789b720a5859b15ac600320279f252b9
22377740/15/1bb8bf9a94ec39b0f2fc61af78f59275cadf10ebf4d30e8ab4b8a11b4cb33ddb
2237d2a0/104/34d3a33fbf552bb81ba0c1b0b998911fca3a9da5f6715ede5c7f76736211667c
2237c190/208/856cd5f6f319236fc4aae3fde8d4378dc0d8f89f810a7ec0faeae9660a163060

CI pending.
