# io.qbs

Qbs: قَبَسَ • (qabasa) I (non-past يَقْبِسُ (yaqbisu), verbal noun قَبْس (qabs))
- to take from, to derive, to obtain (primitively, fire)

## Idea

An implementation for some of golang `io` functionalities but for clang.

## Features

- Copy: copy stream from given reader to given writer with default buffering (512 bytes).
- Copy buffer: copy stream from given reader to given writer but with given buffer.
- Limit reader: read from given reader until n bytes are reached, return error otherwise.
- Read at least: read at least n bytes from given reader into given buffer.
- Read full: read bytes from given reader until the given buffer is full.

## Ignored

- Read all: I did not want to take control on the memory in this lib, so read all is ignored
