#ifndef SYNTIANT_H
#define SYNTIANT_H

/* Pin allocation defines for Syntiant connector SL2 */
#define OUT_1_PIN	15
#define OUT_2_PIN	7
#define OUT_3_PIN	6

/* Macro functions for Pin 5 on SL2 */
#define OUT_1_HIGH()	digitalWrite(OUT_1_PIN, HIGH)
#define OUT_1_LOW()		digitalWrite(OUT_1_PIN, LOW)
#define OUT_1_TOGGLE()	digitalWrite(OUT_1_PIN, !digitalRead(OUT_1_PIN))

/* Macro functions for Pin 6 on SL2 */
#define OUT_2_HIGH()	digitalWrite(OUT_2_PIN, HIGH)
#define OUT_2_LOW()		digitalWrite(OUT_2_PIN, LOW)
#define OUT_2_TOGGLE()	digitalWrite(OUT_2_PIN, !digitalRead(OUT_2_PIN))

/* Macro functions for Pin 7 on SL2 */
#define OUT_3_HIGH()	digitalWrite(OUT_3_PIN, HIGH)
#define OUT_3_LOW()		digitalWrite(OUT_3_PIN, LOW)
#define OUT_3_TOGGLE()	digitalWrite(OUT_3_PIN, !digitalRead(J4_4_PIN))

/* Macro functions for Built in LED */
#define LED_HIGH()		digitalWrite(LED_BUILTIN, HIGH)
#define LED_LOW()		digitalWrite(LED_BUILTIN, LOW)
#define LED_TOGGLE()	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN))

/* Prototypes -------------------------------------------------------------- */
void syntiant_setup(void);
void syntiant_loop(void);

void syntiant_get_imu(float *dest_imu);

#endif