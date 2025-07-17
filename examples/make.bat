cd translator
start /W cmd /c translator.exe %1 "../out.binary" %3 ^1^> last-run.txt ^2^>^&^1

cd "../engine"
start /W cmd /c mediator.exe modules.txt %2 "../out.binary" ^1^> last-run.txt ^2^>^&^1

cd..