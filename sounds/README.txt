Drop your sound effect files here, named exactly:

  move.wav       - played on a normal move
  capture.wav    - played when a piece is captured
  check.wav      - played when a move puts the opponent in check
  gameover.wav   - played on checkmate, stalemate, or time-out

.wav is recommended (raylib also supports .ogg/.mp3 if you rename the
LoadSound(...) calls in utility.cpp accordingly).

If a file is missing, the game won't crash - it just silently skips that
sound (checked via IsSoundValid before every PlaySound call).
