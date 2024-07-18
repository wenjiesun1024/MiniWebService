#Benchmark

echo "GET http://39.98.202.182:9006" | vegeta attack -rate=1000 -connections=1 -duration=3s | tee results.bin | vegeta report
![alt text](image.png)
TODO: 
