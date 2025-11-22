# streams.qbs

Qbs: قَبَسَ • (qabasa) I (non-past يَقْبِسُ (yaqbisu), verbal noun قَبْس (qabs))
- to take from, to derive, to obtain (primitively, fire)

## Idea

Uses an implementation for some of golang `io` functionalities but for clang, and provide an adapting layer for `file` and `tcp` sockets.

## Features

- Basic adapter for `file` operations.
- Basic adapter for `tcp` client operations.

## To Be Done

- Handle `tcp` server and clients connections as streams (reader, writer, closer).
