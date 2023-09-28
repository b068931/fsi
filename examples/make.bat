cd Translator
start /W translator.exe %1 out.binary %3

cd "../Engine"
start /W mediator.exe %2 "../Translator/out.binary"

cd..
cls