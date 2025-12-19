# qbs

Qbs: قَبَسَ • (qabasa) I (non-past يَقْبِسُ (yaqbisu), verbal noun قَبْس (qabs))
- to take from, to derive, to obtain (primitively, fire)

## Idea

- An implementation for some of `Golang` `io` functionalities but for `C`.
- Provide an adapter layer for `bytes`, `file` and `tcp` sockets so they integrate with `io`.

## Features

- Copy stream from given reader to given writer with default buffering (512 bytes).
- Copy stream from given reader to given writer but with given buffer.
- Limit reader, which read from given reader until n bytes are reached, return error otherwise.
- Read at least, which read at least n bytes from given reader into given buffer.
- Read full, which read bytes from given reader until the given buffer is full.
- Basic adapter for `file` operations.
- Basic adapter for `tcp` client operations.
- Basic adapter for `tcp` server operations.
- Basic adapter for `bytes` operations.

## TBD

- Handle windows socket for `tcp`.
- Write a simple http client that handle `post`, `get` request, it should accept the request body reader as a parameter and return the response body as a reader.

