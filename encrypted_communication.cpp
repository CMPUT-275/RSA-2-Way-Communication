#include <Arduino.h>


void setup() {
	init();  // Initialize serial
	pinMode(13, INPUT);  // Determine Arduino 1 and 2
	Serial.begin(9600);  // User input
	Serial3.begin(9600);  // Arduino input
}


bool wait_on_serial3( uint8_t nbytes, long timeout ) {
/*
	Looks for available bytes until it timesout

	Argument:
		nbytes - number of required bytes
		timeout - value of timeout
*/
	unsigned long deadline = millis() + timeout;
	while (Serial3.available()<nbytes && (timeout < 0 || millis() < deadline)) {
		delay(1);
	}
	return Serial3.available()>=nbytes;
}


void uint32_to_serial3(uint32_t num) { 
/*
	Writes a 32 bit to Serial3 in 4 bytes

	Argument:
		num - the byte to write
*/
	Serial3.write((char) (num >> 0));
	Serial3.write((char) (num >> 8));
	Serial3.write((char) (num >> 16));
	Serial3.write((char) (num >> 24));
}


uint32_t uint32_from_serial3() {
/*
	Reads a 32 bit from Serial3 to Serial
*/
	uint32_t num = 0;
	num = num | ((uint32_t) Serial3.read()) << 0;
	num = num | ((uint32_t) Serial3.read()) << 8;
	num = num | ((uint32_t) Serial3.read()) << 16;
	num = num | ((uint32_t) Serial3.read()) << 24;
	return num;
}


bool checkPrime(uint32_t n) {
/*
	Checks if n is a prime number

	Arguments:
		n - number to check

	Returns:
		bool of whether it is a prime (true) or not (false)
*/
	if (n == 1 || n == 2) {
		return false;
	} else {
		for (int i = 2; i <= sqrt(n); i++) {
			if (n % i == 0) {
				return false;
			}
		}
		return true;
	}
}


uint32_t mulmod(uint32_t byte1, uint32_t byte2, uint32_t m) {
/*
	Modular Multiplication

	Arguments:
		byte1 - muliplier
		byte2 - multiplicant
		m - Given Modulus
	Returns:
		result - product of byte1 and byte2
*/
		uint32_t result = 0;
		for (uint32_t i = 0; i<32 ; i++) {
			if (byte1 & 1){
				result = (result+byte2)%m;
			}
			byte2 = (byte2*2)%m;
			byte1 = byte1 >> 1; // Halves byte1
		}
		return result;
}	


uint32_t powMod(uint32_t byte, uint32_t Key, uint32_t Mod) {
/*
	Modular Power

	Arguments:
		byte: base
		Key: exponent
		Mod: Modulus
	Returns:
		newByte: The result of the power
*/
	uint32_t newByte = 1;
	while (Key > 0) {

		if (Key & 1) {
			
			newByte = mulmod(newByte, byte, Mod); // byte times newByte

		}
		byte = mulmod(byte, byte, Mod); // Byte squared
		Key = Key >> 1; // Halves Key


	}
	return newByte;
}


enum ServerState { Start, Listen, WaitingForKey, WaitingForAck, DataExchange };


uint32_t negativeMod(int32_t x, uint32_t mod) {
/*
	Computes the modulus of a negative input

	Arguments:
		x - a negative number
		mod - the modulus

	Returns:
		x % mod
*/
	uint32_t z;
	z = -x/mod +1;
	return x + z*mod;
}


uint32_t phiN(uint32_t p, uint32_t q) {
/*
	computes phiN
*/
	return (p-1)*(q-1);
}


uint32_t randNum(uint32_t bits) {
/*
	Generates a random number

	Arguments:
		bits - the number of bits of the generated number

	Returns:
		randomByte - the random number
*/
	uint32_t randomByte = 0;
	for (int i = 0; i < bits-1; i++) {
		delay(5);
		randomByte |= ((uint32_t)(analogRead(A1) & 1) << i);
	}
	randomByte = (randomByte | ((uint32_t)1 << (bits-1)));
	return randomByte;
}


uint32_t gcd(uint32_t a, uint32_t b) {
/*
	Computed Greatest Common Demoninator

	Arguments:
		a - one number
		b - another number
	Returns:
		the greatest common demoninator of a and b
*/
	while (b > 0) {
		a = a % b;
		uint32_t tmp = a;
		a = b;
		b = tmp;
	}
	return a;
}


int32_t extendedEuclidean(uint32_t e, uint32_t n) {
/*
	Generates d from given e and n

	Arguments:
		e - The public key
		n - modulus
	Returns:
		the private key (d)
*/
	int32_t r[40];
	int32_t q;
	int32_t s[40];
	int32_t t[40];
	int32_t i = 1;
	//Setting the intial matrix
	r[0] = e;
	r[1] = n;
	s[0] = 1;
	s[1] = 0;
	t[0] = 0;
	t[1] = 1;
	while (r[i] > 0) {
		q = r[i-1]/r[i];
		r[i+1] = r[i-1] - q * r[i];
		s[i+1] = s[i-1] - q * s[i];
		t[i+1] = t[i-1] - q * t[i];
		
		i++;
	}
		return s[i-1];	
}


uint32_t generatePrime(uint32_t length) {
/*
	Generates a prime

	Argument:
		length - the number of bits of the prime

	Returns:
		the prime
*/
	bool prime = false;
	uint32_t primeByte;
	while (prime == false) {
		primeByte = randNum(length);
		prime = checkPrime(primeByte);
	}
	return primeByte;
}


void generateKeys(uint32_t& e, int32_t& d, uint32_t& n) {
/*
	Generates Keys

	Arguments:
		e, d, n = placeholders for its public key, private key, and modulus

*/
	uint32_t p = 0, q = 0, phi = 0;
	(int32_t) d;
	e = generatePrime(15);
	q = generatePrime(16);
	p = generatePrime(15);
	n = p * q;
	phi = phiN(p,q);
	while (gcd(e, phi) != 1) {
		Serial.println("e failed!");
		Serial.println("Genernating new e...");
		e = generatePrime(16);
	}
	d = extendedEuclidean(e, phi);
	if (d < 0 or d >= phi) {
		d = negativeMod(d, phi);
	}

}


void communicate(uint32_t e, uint32_t m, uint32_t d, uint32_t n) {
/*
	Encryptes and sends bytes and Decryptes recieved bytes

	Arguments:
		e: otherPublicKey
		m: otherModulus
		d: ownPrivateKey
		n: ownModulus
*/
	uint32_t en_byte;  // Encrypte Byte
	uint8_t de_byte;  // Dencrypte Byte


	while (true) {

		if (Serial.available() > 0 ) { // User writes input
			uint32_t readByte = Serial.read();

			Serial.print(char(readByte));  // Write to monitor
			en_byte = powMod(char(readByte), e, m);  // Encrypt

			uint32_to_serial3(en_byte);  // Write encrypt value to the other Arduino
			if(readByte == '\r') {  // If user inputs return
				Serial.write('\n');
				en_byte = powMod('\n', e, m);
				uint32_to_serial3(en_byte);
				}
			
		} else if (Serial3.available() > 3) {  // Arduino recieving input from other Arduino
			en_byte = uint32_from_serial3();  // Read input from Arduino
			de_byte = powMod(en_byte, d, n);  // Dencrypt
			Serial.write(de_byte);  // Write Dencrypt value
		}

	}
}
void synchronize(){
/*
	This is where the client and the server declares their keys and mods
	They then hand shake to synchronize
	Then goes to next function where they communicate
*/
	uint32_t e = 0, n = 0, m = 0, e2 = 0;  // e and e2 are public keys, n and m are mods
	int32_t d = 0;  // Private key
	uint32_t timeout = 1000;// should put with initial variables
	ServerState state = Listen;

	if (digitalRead(13) == HIGH) {  // This will be client
		state = Start;
		Serial.println("");
		Serial.println("-------------Client----------------");
		Serial.println("Genernating Keys...");
		generateKeys(e,d,n);  // Generates keys
		Serial.println("Keys have been Generated!");
		while (true) {
			if (state == Start){  // Outputs ckey cmod
				Serial.println("Waiting for connection...");
				Serial3.write('C');  // Sends connection request
				uint32_to_serial3(e);  // Key
				uint32_to_serial3(n);  // Mod
				state = WaitingForAck;

			}
			else if (state == WaitingForAck){
				if (wait_on_serial3(9, timeout)){
					if (Serial3.read() == 'A'){  // Recives Acknolwdgment from server
						e2 = uint32_from_serial3();  // Key
						m = uint32_from_serial3();  // Mod
						Serial3.write('A');  // Sends Acknowledgement
						state = DataExchange;
						Serial.println("Recived connection...");
					}
				} 
				else {
					state = Start;  // Timeout
				}
			}
			else if (state = DataExchange){
				Serial.println("Chat is open:");
				delay(500);// ensure all bytes are written
				// Clears Serial3 before chatting
				while (Serial3.available() > 0) {
					Serial3.read();
				}
				
				communicate(e2,m,d,n);  // Chat
			}
		}
	} else {  // This will be server
		state = Listen;
		Serial.println("");
		Serial.println("------------Server-------------");
		Serial.println("Genernating Keys...");
		generateKeys(e,d,n);  // Generates keys
		Serial.println("Keys have been Generated!");
		while (true){
			if (state == Listen){  // Recive connection request
				Serial.println("Listening...");
				if (wait_on_serial3(9, 1000)){
					if (Serial3.read() == 'C'){  // 'C' and Key and Mod
						Serial.println("Recived connection...");
						state = WaitingForKey;
					}
				}	
			}
			else if (state == WaitingForKey){ // Recieves keys
					Serial3.write('A');  // Sends Acknowledgement
					uint32_to_serial3(e);  // Key
					uint32_to_serial3(n);  // Mod
					e2 = uint32_from_serial3();  // Key
					m = uint32_from_serial3();  // Mod

					state = WaitingForAck;
			}
			else if (state == WaitingForAck){  // Check for acknowledgement
				if (wait_on_serial3(1, 1000)){  // If it recives something
					char input = Serial3.read();
					if (input == 'C' && Serial3.available() >= 8){  // 'C' and Key and Mod
						Serial.println("Recived connection...");
						state = WaitingForKey;
					}
					else if (input == 'A') {  // Acknowledgement
						Serial.println("Recived acknowledgement...");
						state = DataExchange;
					}
				}
				else {
					state = Listen;  // Timeout
				}
			} 
			else if (state == DataExchange){  // Initialize communication
				Serial.println("Chat is open:");
				delay(500); // ensure all bytes are written
				// clears out Serial3 before chatting
				while (Serial3.available() > 0) {
					Serial3.read();
				}
				communicate(e2,m,d,n);  // Chat
			}
		}
	}
}

int main() {
	setup();
	synchronize();  // Key generation and handshake
	Serial.flush();
	Serial3.flush();
	return 0;
}