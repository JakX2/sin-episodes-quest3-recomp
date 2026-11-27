# Phase 1606: Minor leap and airborne think

Reopened original private server.dll, SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Verified Minor +76c ->2237d890. Mode0 clamps live gravity to >=1, refreshes dirty origin, calls original shared 2204c850(origin,target,gravity,3). Mode1 takes owner forward basis from actor angles+36c via 22269bc0 and scales by350. Both call 2237d7a0: installs contact marker 2237c980 at +7c, sets current58, clears ground, writes +c38 now+.5, relocates +360 Z+1, sets velocity, conditionally refreshes +2bc, copies current origin to +c24, sets +c41, installs airborne think 2237c2c0 and schedules now. Airborne think performs inherited +418 only if now>+c34, sets deadline now+0.1 (double constant); grounded +bc&1 restores default think22044fa0 and schedules now+0.1, otherwise schedules current time. No generic physics/animation recreated; owner executes actual velocity, movement, ballistic and inherited AI services. Native explicit helper; original animation event can bind it through launch callback. Loaded regression covers both launch modes, order, exact think deadline, live snapshot and grounded return.

VA/bytes/SHA256:
2237d7a0/238/7e30fe3df4097cb869371b7054db8d6311da3b2fdb9f8153b00a1bc47b0e06b3
2237d890/248/6f565d4073d6593a38f9c719af78e8e3ce09f22473f84af6e227639cfe2f89be
2237c2c0/160/b75476b8db49d7155efa2c0c34aa2275c995d0933a67aca4028857a8d2bc9aaf

CI pending. No original executable bytes committed.
