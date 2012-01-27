file(READ ${fixfile} code)
string(REPLACE "int i = strerror_r(0, buf, 100);" 
"buf = strerror_r(0, buf, 100)" code "${code}")
file(WRITE ${fixfile} "${code}")
