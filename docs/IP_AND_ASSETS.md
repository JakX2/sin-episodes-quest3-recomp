# Intellectual property, binary evidence and game assets

This repository is an independent, unofficial compatibility and reverse-
engineering effort. It is **not affiliated with, sponsored by, or endorsed by**
Ritual Entertainment, Valve, or the current game rights holders. Product and
company names are descriptive references to their respective owners.

## What this repository contains

- New C++17 compatibility code, host-adapter interfaces and tests.
- Behavioral reference headers authored from prior research packages, with
  provenance and known limitations recorded in
  `third_party/research/PROVENANCE.md`.
- Instructions for independently verifying behavior against lawfully obtained
  software where local law permits.

## What this repository must not contain

- Original SiN Episodes or Half-Life 2 `server.dll` or `client.dll` binaries.
- Game install files, archives, maps, character or weapon models, textures,
  sounds, music, proprietary engine libraries, or unlicensed SDK code.
- Decompiled reproductions or copied code whose distribution rights are unclear.
- Reverse-engineering data that unnecessarily embeds copyrighted executable
  content; prefer small behavioral descriptions, function IDs, test vectors and
  independently reproducible tools.

The MIT license in the root of this repo covers independently authored project
source to the extent its contributors have the rights to license it. It **does
not license the original game, engine, game assets, third-party trademarks or
code owned by others**. Third-party integrations need their own license review.

## How to test against original games

Use a legally obtained game installation outside the repository. Never commit
those files, credentials, extraction outputs, or a packaged build containing
third-party assets. Test on controlled local fixtures and publish descriptions
of expected behavior rather than redistribution of proprietary files.
