cbmc --version
echo %date% %time%
cbmc --unwind 10 --unwinding-assertions main2_Bounded.c >> 10iter.txt 
echo %date% %time%
cbmc --unwind 50 --unwinding-assertions main2_Bounded.c >> 50iter.txt
echo %date% %time%
cbmc --unwind 100 --unwinding-assertions main2_Bounded.c >> 100iter.txt
echo %date% %time%