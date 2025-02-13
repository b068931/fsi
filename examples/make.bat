cd Translator
start /W translator.exe %1 "../out.binary" %3

cd "../Engine"
start /W mediator.exe modules.txt %2 "../out.binary"

cd..
cls