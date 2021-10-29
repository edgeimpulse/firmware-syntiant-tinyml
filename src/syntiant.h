#ifndef SYNTIANT_H
#define SYNTIANT_H

/* Pin allocation defines for Syntiant connector J4 */
#define J4_2_PIN	2
#define J4_3_PIN	21
#define J4_4_PIN	11
#define J4_5_PIN	12

/* Macro functions for Pin 2 on J4 */
#define J4_2_HIGH()		digitalWrite(J4_2_PIN, HIGH)
#define J4_2_LOW()		digitalWrite(J4_2_PIN, LOW)
#define J4_2_TOGGLE()	digitalWrite(J4_2_PIN, !digitalRead(J4_2_PIN))

/* Macro functions for Pin 3 on J4 */
#define J4_3_HIGH()		digitalWrite(J4_3_PIN, HIGH)
#define J4_3_LOW()		digitalWrite(J4_3_PIN, LOW)
#define J4_3_TOGGLE()	digitalWrite(J4_3_PIN, !digitalRead(J4_3_PIN))

/* Macro functions for Pin 4 on J4 */
#define J4_4_HIGH()		digitalWrite(J4_4_PIN, HIGH)
#define J4_4_LOW()		digitalWrite(J4_4_PIN, LOW)
#define J4_4_TOGGLE()	digitalWrite(J4_4_PIN, !digitalRead(J4_4_PIN))

/* Macro functions for Pin 5 on J4 */
#define J4_5_HIGH()		digitalWrite(J4_5_PIN, HIGH)
#define J4_5_LOW()		digitalWrite(J4_5_PIN, LOW)
#define J4_5_TOGGLE()	digitalWrite(J4_5_PIN, !digitalRead(J4_5_PIN))

#define LED_HIGH()		digitalWrite(LED_BUILTIN, HIGH)
#define LED_LOW()		digitalWrite(LED_BUILTIN, LOW)
#define LED_TOGGLE()	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN))

/* Prototypes -------------------------------------------------------------- */
void syntiant_setup(void);
void syntiant_loop(void);

#endif