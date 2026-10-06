#ifndef DHT11_READER_H
#define DHT11_READER_H

int  dht11_init(void);
int  dht11_read(float *temp, float *humidity);
void dht11_cleanup(void);

#endif
