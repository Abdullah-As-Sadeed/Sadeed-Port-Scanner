# Sadeed-Port-Scanner
Scans ports from 0 to 65535 of a target server and reports their status and separately mentions the open ports. Works on Linux.

## Compilation
Use g++.
```
g++ ./Sadeed_Port_Scanner.cpp -o ./Sadeed_Port_Scanner
```

## Usage
Give the domain or IP address of the target server as an argument.
```
./Sadeed_Port_Scanner localhost
```
```
./Sadeed_Port_Scanner example.com
```
```
./Sadeed_Port_Scanner 103.108.140.126
```

After scanning each port, open ports will be shown. At the end, open ports will be mentioned separately.  
If the program is interrupted by SIGINT signal, the found open ports will be mentioned also.
