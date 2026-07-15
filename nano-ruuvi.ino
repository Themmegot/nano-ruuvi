/*
 * Ruuvi tag emulator — Arduino Nano 33 BLE Sense (rev1)
 *
 * Broadcasts the onboard HTS221 (temp/humidity), LPS22HB (pressure) and
 * LSM9DS1 (movement counter) readings as Ruuvi RAWv2 (data format 5) BLE
 * manufacturer advertisements. Anything that understands Ruuvi tags —
 * including a Victron Cerbo GX with Bluetooth sensors enabled, or the
 * Ruuvi Station phone app — will pick it up as a wireless climate sensor.
 *
 * Serial (115200) prints each broadcast for debugging.
 */

#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_LSM9DS1.h>

const uint16_t ADV_INTERVAL_MS = 1285;  // Ruuvi's standard cadence

uint8_t movementCounter = 0;
uint16_t sequence = 0;
float lastAx = 0, lastAy = 0, lastAz = 1;

void putBE16(uint8_t *p, uint16_t v) {
  p[0] = v >> 8;
  p[1] = v & 0xFF;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== Ruuvi tag emulator (Nano 33 BLE Sense) ===");

  bool ok = true;
  if (!HTS.begin())  { Serial.println("HTS221 (temp/hum) init failed");  ok = false; }
  if (!BARO.begin()) { Serial.println("LPS22HB (pressure) init failed"); ok = false; }
  if (!IMU.begin())  { Serial.println("LSM9DS1 (IMU) init failed");      ok = false; }
  if (!BLE.begin())  { Serial.println("BLE init failed");                ok = false; }
  if (!ok) {
    Serial.println("Halted - check board revision (rev1 sensors expected).");
    while (true) delay(1000);
  }

  BLE.setLocalName("Ruuvi ENV");
  Serial.println("BLE address: " + BLE.address());
  Serial.println("Broadcasting RAWv2 every " + String(ADV_INTERVAL_MS) + " ms");
}

void loop() {
  float temp = HTS.readTemperature();      // deg C
  float hum = HTS.readHumidity();          // %RH
  float pressPa = BARO.readPressure() * 1000.0f;  // kPa -> Pa

  float ax = lastAx, ay = lastAy, az = lastAz;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    // Count "movement" events like a Ruuvi does: significant accel change
    if (fabsf(ax - lastAx) + fabsf(ay - lastAy) + fabsf(az - lastAz) > 0.15f)
      movementCounter++;
    lastAx = ax; lastAy = ay; lastAz = az;
  }

  // ---- Ruuvi data format 5 (RAWv2) payload
  uint8_t mfg[26];
  mfg[0] = 0x99;  // manufacturer ID 0x0499 (Ruuvi Innovations), little-endian
  mfg[1] = 0x04;
  uint8_t *d = mfg + 2;
  d[0] = 0x05;                                        // format 5
  putBE16(d + 1, (int16_t)roundf(temp / 0.005f));     // 0.005 degC/LSB
  putBE16(d + 3, (uint16_t)roundf(hum / 0.0025f));    // 0.0025 %/LSB
  putBE16(d + 5, (uint16_t)roundf(pressPa - 50000));  // Pa - 50000
  putBE16(d + 7, (int16_t)roundf(ax * 1000));         // mg
  putBE16(d + 9, (int16_t)roundf(ay * 1000));
  putBE16(d + 11, (int16_t)roundf(az * 1000));
  // Power info: battery 3300 mV (USB), TX power +4 dBm
  uint16_t power = ((uint16_t)((3300 - 1600) / 1) << 5) | (uint16_t)((4 + 40) / 2);
  putBE16(d + 13, power);
  d[15] = movementCounter;
  putBE16(d + 16, sequence++);
  // MAC address, big-endian
  String mac = BLE.address();
  for (int i = 0; i < 6; i++)
    d[18 + i] = strtoul(mac.substring(i * 3, i * 3 + 2).c_str(), nullptr, 16);

  BLE.stopAdvertise();
  BLE.setManufacturerData(mfg, sizeof(mfg));
  BLE.advertise();

  Serial.print("T=" + String(temp, 2) + "C  RH=" + String(hum, 1) + "%  P=" +
               String(pressPa / 100.0f, 1) + "hPa");
  Serial.println("  move=" + String(movementCounter) + "  seq=" + String(sequence));

  delay(ADV_INTERVAL_MS);
}
