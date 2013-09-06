// Функция передачи файлов, вызывать периодически
void transfer_data();

// Вызывать для начала передачи файлов
void transfer_start(unsigned long int number, unsigned char address);

// Подтверждение приёма пакета
void transfer_ack();

// Прекращение передачи
void transfer_stop();
