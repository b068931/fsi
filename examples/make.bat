cd translator
start /W translator.exe %1 "../out.binary" %3

cd "../engine"
start /W mediator.exe modules.txt %2 "../out.binary"

cd..
cls