cd translator
start /W cmd /c translator.exe %1 "../out.bfsi" %3 ^1^> last-run.txt ^2^>^&^1

cd "../engine"
start /W cmd /c mediator.exe engine.mods %2 "../out.bfsi" ^1^> last-run.txt ^2^>^&^1

cd..