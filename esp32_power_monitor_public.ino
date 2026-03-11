// ============================================================
//  ESP32 WROOM-32 — Smart Power Monitor & PF Corrector
//  Features: Blynk + Adafruit IO + PFC + Energy Monitoring
//  AC Mains: 220V / 50Hz | Timezone: Asia/Kolkata (IST)
// ============================================================

// ============================================================
//  SECTION 1: INCLUDES
// ============================================================
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <AdafruitIO_WiFi.h>
#include <time.h>
#include <Preferences.h>
#include <math.h>

// ============================================================
//  SECTION 2: CREDENTIALS — SUBSTITUTE YOUR VALUES HERE
// ============================================================
#define WIFI_SSID        "YOUR_WIFI_SSID"              // <-- substitute
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"          // <-- substitute
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"       // <-- substitute
#define AIO_USERNAME     "YOUR_ADAFRUIT_IO_USERNAME"   // <-- substitute
#define AIO_KEY          "YOUR_ADAFRUIT_IO_KEY"        // <-- substitute

// ============================================================
//  SECTION 3: PIN DEFINITIONS
// ============================================================
#define VOLTAGE_PIN      34    // ZMPT101B output
#define CURRENT_PIN      35    // ACS712 30A output
#define ZCD_PIN          32    // Zero Cross Detection (interrupt)

// Capacitor bank relay pins (C1=1µF, C2=2µF, C3=3µF,
//                            C4=4µF, C5=5µF, C6=6µF, C7=15µF)
#define CAP_PIN_C1       25
#define CAP_PIN_C2       26
#define CAP_PIN_C3       27
#define CAP_PIN_C4       14
#define CAP_PIN_C5       12
#define CAP_PIN_C6       13
#define CAP_PIN_C7       33

// ============================================================
//  SECTION 4: CONSTANTS
// ============================================================
#define SAMPLES          200
#define MAINS_FREQ       50.0f
#define SAMPLE_INTERVAL  100        // µs → 10kHz sampling rate
#define ADC_MAX          4095.0f
#define ADC_VREF         3.3f
#define ZMPT_VOFFSET     (ADC_VREF / 2.0f)   // 1.65V midpoint
#define ZMPT_SCALE       428.0f               // Calibrate for 220V RMS
#define ACS_VOFFSET      (ADC_VREF / 2.0f)   // 1.65V midpoint
#define ACS_SENSITIVITY  0.066f               // ACS712-30A: 66mV/A
#define MIN_CURRENT      0.05f               // Below this = no load
#define PF_DEADBAND_LOW  0.95f               // No correction above this
#define PF_BAD_CYCLES    3                   // Cycles before correction
#define NUM_CAPS         7

// NTP
#define NTP_SERVER       "pool.ntp.org"
#define GMT_OFFSET_SEC   19800               // IST = UTC+5:30
#define DAYLIGHT_OFFSET  0

// Blynk virtual pins
#define V_VOLTAGE        V0
#define V_CURRENT        V1
#define V_POWER          V2
#define V_CAP1           V3
#define V_CAP2           V4
#define V_CAP3           V5
#define V_CAP4           V6
#define V_CAP5           V7
#define V_CAP6           V8
#define V_CAP7           V9
#define V_ENERGY         V11
#define V_APPARENT       V12
#define V_REACTIVE       V13
#define V_PF             V14
#define V_FREQ           V15
#define V_COMPLEX_S      V16
#define V_COMPLEX_Z      V17
#define V_DC_VOLT        V18
#define V_DC_CURR        V19
#define V_CREST_V        V20
#define V_CREST_C        V21
#define V_FORM_V         V22
#define V_FORM_C         V23
#define V_STDDEV_V       V24
#define V_STDDEV_C       V25
#define V_LOAD_TYPE      V26
#define V_INRUSH         V27
#define V_CHIP_TEMP      V28
#define V_MONTHLY        V29
#define V_MIN_VOLT       V30
#define V_MAX_VOLT       V31
#define V_MIN_CURR       V32
#define V_MAX_CURR       V33
#define V_RESET_EXT      V34
#define V_TS_MIN_V       V35
#define V_TS_MAX_V       V36
#define V_TS_MIN_C       V37
#define V_TS_MAX_C       V38
#define V_MIN_FREQ       V39
#define V_MAX_FREQ       V40
#define V_MIN_PF         V41
#define V_MAX_PF         V42
#define V_TS_MIN_F       V43
#define V_TS_MAX_F       V44
#define V_TS_MIN_PF      V45
#define V_TS_MAX_PF      V46
#define V_PF_LABEL       V47
#define V_RESISTANCE     V48
#define V_REACTANCE      V49
#define V_RESET_ENERGY   V10
#define V_PFC_COUNT      V56
#define V_RESET_PFC      V58
#define V_MAX_INRUSH     V59
#define V_RESET_INRUSH   V57
#define V_TS_INRUSH      V60
#define V_TS_PFC         V61
#define V_TARGET_CAP     V62    // Display calculated target µF

// ============================================================
//  SECTION 5: ADAFRUIT IO FEEDS
// ============================================================
AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, WIFI_SSID, WIFI_PASSWORD);

AdafruitIO_Feed *voltFeed         = io.feed("voltage");
AdafruitIO_Feed *currFeed         = io.feed("current");
AdafruitIO_Feed *powerFeed        = io.feed("realpower");
AdafruitIO_Feed *pfFeed           = io.feed("powerfactor");
AdafruitIO_Feed *freqFeed         = io.feed("frequency");
AdafruitIO_Feed *energyFeed       = io.feed("energy");
AdafruitIO_Feed *dcVoltageFeed    = io.feed("dcvoltage");
AdafruitIO_Feed *dcCurrentFeed    = io.feed("dccurrent");
AdafruitIO_Feed *crestVoltageFeed = io.feed("crestvoltage");
AdafruitIO_Feed *crestCurrentFeed = io.feed("crestcurrent");
AdafruitIO_Feed *formVoltageFeed  = io.feed("formvoltage");
AdafruitIO_Feed *formCurrentFeed  = io.feed("formcurrent");
AdafruitIO_Feed *stddevVoltageFeed= io.feed("stddevvoltage");
AdafruitIO_Feed *stddevCurrentFeed= io.feed("stddevcurrent");
AdafruitIO_Feed *loadTypeFeed     = io.feed("loadtype");
AdafruitIO_Feed *inrushRatioFeed  = io.feed("inrushratio");
AdafruitIO_Feed *maxInrushFeed    = io.feed("maxinrush");
AdafruitIO_Feed *pfCorrCountFeed  = io.feed("pfcorrectioncount");
AdafruitIO_Feed *chipTempFeed     = io.feed("chiptemp");
AdafruitIO_Feed *monthlyEstFeed   = io.feed("monthlyestimate");

// ============================================================
//  SECTION 6: GLOBAL VARIABLES
// ============================================================

// --- Sampling buffers ---
float voltageSamples[SAMPLES];
float currentSamples[SAMPLES];

// --- Measured values ---
float voltage        = 0.0f;
float current        = 0.0f;
float realPower      = 0.0f;
float apparentPower  = 0.0f;
float reactivePower  = 0.0f;
float powerFactor    = 0.0f;
float energyKWh      = 0.0f;
float voltageDC      = 0.0f;
float currentDC      = 0.0f;
float voltagePeak    = 0.0f;
float currentPeak    = 0.0f;
float resistance     = 0.0f;
float reactance      = 0.0f;
float estimatedMonthlyEnergy = 0.0f;
volatile float measuredFrequency = 50.0f;

// --- Waveform quality metrics ---
float crestFactorV   = 0.0f;
float crestFactorC   = 0.0f;
float formFactorV    = 0.0f;
float formFactorC    = 0.0f;
float stddevV        = 0.0f;
float stddevC        = 0.0f;

// --- String labels ---
String pfLabel       = "";
String complexPower  = "";
String complexImpedance = "";
String loadType      = "";

// --- Extremes ---
float minVoltage     = 9999.0f;
float maxVoltage     = 0.0f;
float minCurrent     = 9999.0f;
float maxCurrent     = 0.0f;
float minFrequency   = 9999.0f;
float maxFrequency   = 0.0f;
float minPF          = 1.0f;
float maxPF          = 0.0f;

// --- Extreme timestamps (Unix time) ---
unsigned long tsMinV  = 0, tsMaxV  = 0;
unsigned long tsMinC  = 0, tsMaxC  = 0;
unsigned long tsMinF  = 0, tsMaxF  = 0;
unsigned long tsMinPF = 0, tsMaxPF = 0;

// --- Inrush & PFC ---
float    inrushRatio      = 0.0f;
float    maxInrushRatio   = 0.0f;
int      pfCorrectionCount = 0;
unsigned long tsMaxInrush = 0;
unsigned long tsLastPFC   = 0;
bool     applyCorrectionFlag = false;

// --- Zero cross detection (ISR) ---
volatile unsigned long lastVoltageZeroTime    = 0;
volatile unsigned long currentVoltageZeroTime = 0;
volatile bool          zeroCrossed            = false; // available for future use

// --- Power factor crossing (sample index for phase shift) ---
int  vZeroTime = 0, cZeroTime = 0;
bool vZeroFound = false, cZeroFound = false;

// --- PFC Zero Voltage Switching state machine ---
// Tracks how many consecutive cycles PF has been bad
int      pfBadCycleCount    = 0;
volatile int  currentCapacitanceUF = 0;     // µF currently connected
volatile bool pendingCapSwitch    = false;  // waiting for zero cross to switch
volatile bool pendingCapRemove    = false;  // waiting for zero cross to remove
int      pendingCapTarget    = 0;           // target µF for next switch

// --- Capacitor bank ---
// C1=1µF, C2=2µF, C3=3µF, C4=4µF, C5=5µF, C6=6µF, C7=15µF
const int  capValues[NUM_CAPS] = {1, 2, 3, 4, 5, 6, 15};
const int  capPins[NUM_CAPS]   = {CAP_PIN_C1, CAP_PIN_C2, CAP_PIN_C3,
                                   CAP_PIN_C4, CAP_PIN_C5, CAP_PIN_C6,
                                   CAP_PIN_C7};

// Lookup table in IRAM for safe ISR access
// capCombinations[n] = bitmask of caps for n µF
static const uint8_t IRAM_ATTR capCombinations[37] = {
  0b0000000, // 0µF  — all off
  0b0000001, // 1µF  — C1
  0b0000010, // 2µF  — C2
  0b0000100, // 3µF  — C3
  0b0001000, // 4µF  — C4
  0b0010000, // 5µF  — C5
  0b0100000, // 6µF  — C6
  0b0100001, // 7µF  — C1+C6
  0b0100010, // 8µF  — C2+C6
  0b0100100, // 9µF  — C3+C6
  0b0101000, // 10µF — C4+C6
  0b0110000, // 11µF — C5+C6
  0b0110001, // 12µF — C1+C5+C6
  0b0110010, // 13µF — C2+C5+C6
  0b0110100, // 14µF — C3+C5+C6
  0b1000000, // 15µF — C7
  0b1000001, // 16µF — C1+C7
  0b1000010, // 17µF — C2+C7
  0b1000100, // 18µF — C3+C7
  0b1001000, // 19µF — C4+C7
  0b1010000, // 20µF — C5+C7
  0b1100000, // 21µF — C6+C7
  0b1100001, // 22µF — C1+C6+C7
  0b1100010, // 23µF — C2+C6+C7
  0b1100100, // 24µF — C3+C6+C7
  0b1101000, // 25µF — C4+C6+C7
  0b1110000, // 26µF — C5+C6+C7
  0b1110001, // 27µF — C1+C5+C6+C7
  0b1110010, // 28µF — C2+C5+C6+C7
  0b1110100, // 29µF — C3+C5+C6+C7
  0b1111000, // 30µF — C4+C5+C6+C7
  0b1111001, // 31µF — C1+C4+C5+C6+C7
  0b1111010, // 32µF — C2+C4+C5+C6+C7
  0b1111100, // 33µF — C3+C4+C5+C6+C7
  0b1111101, // 34µF — C1+C3+C4+C5+C6+C7
  0b1111110, // 35µF — C2+C3+C4+C5+C6+C7
  0b1111111  // 36µF — ALL
};

// --- Timing ---
unsigned long lastEnergyUpdate   = 0;
unsigned long lastBlynkUpdate    = 0;
unsigned long lastAdafruitUpdate = 0;
float         previousCurrent    = 0.0f;

// --- Energy tracking ---
unsigned long energyStartMs = 0;  // millis() at energy reset or boot
Preferences preferences;

// ============================================================
//  SECTION 7: FUNCTION PROTOTYPES
// ============================================================
float   calculateRMS(float* samples, int count);
float   calculateMean(float* samples, int count);
float   calculateStdDev(float* samples, int count, float mean);
void    takeSamples();
void    computeMeasurements();
void    updateExtremes();
void    updateEnergy();
void    computeWaveformMetrics();
void    detectLoadType();
void    detectInrush();
void    computePowerFactor();
void    applyPFCorrectionLogic();
void    applyCapacitorMask(uint8_t mask);
void    updateBlynk();
void    updateAdafruitIO();
void    saveExtremesToNVS();
void    saveAdvancedToNVS();
void    loadFromNVS();
String  getFormattedTime(unsigned long unixTime);
int     calculateRequiredCapacitance(float pf, float v, float i);
IRAM_ATTR void voltageZeroCross();

// ============================================================
//  SECTION 8: ISR — ZERO CROSS DETECTION
// ============================================================
void IRAM_ATTR voltageZeroCross() {
  unsigned long t = micros();

  // Debounce: minimum 8ms between valid crossings (handles up to 62.5Hz)
  if (t - currentVoltageZeroTime > 8000) {
    lastVoltageZeroTime    = currentVoltageZeroTime;
    currentVoltageZeroTime = t;

    // Calculate frequency from half-period (each crossing = half cycle)
    unsigned long periodMicros = (currentVoltageZeroTime - lastVoltageZeroTime) * 2;
    if (periodMicros > 1000) {
      measuredFrequency = 1e6f / periodMicros;
    }

    zeroCrossed = true;  // available for future use if needed

    // --- Zero Voltage Switching: apply or remove caps at zero cross ---
    if (pendingCapSwitch) {
      applyCapacitorMask(capCombinations[pendingCapTarget]);
      currentCapacitanceUF = pendingCapTarget;
      pendingCapSwitch     = false;
    }
    if (pendingCapRemove) {
      applyCapacitorMask(0x00);  // all caps off
      currentCapacitanceUF = 0;
      pendingCapRemove     = false;
    }
  }
}

// ============================================================
//  SECTION 9: SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Power Monitor — Booting...");

  // Capacitor relay pins — default OFF
  for (int i = 0; i < NUM_CAPS; i++) {
    pinMode(capPins[i], OUTPUT);
    digitalWrite(capPins[i], LOW);
  }

  // Zero cross interrupt — RISING edge (optocoupler output)
  pinMode(ZCD_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), voltageZeroCross, RISING);

  // ADC resolution
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // 0–3.3V range

  // Load saved values from NVS
  loadFromNVS();

  // Connect WiFi + Blynk
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // NTP time sync
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
  Serial.print("Syncing NTP time");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nNTP synced.");

  // Adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();
  while (io.status() < AIO_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nAdafruit IO connected.");

  lastEnergyUpdate = millis();
  energyStartMs    = millis();
  Serial.println("Setup complete. Running...");
}

// ============================================================
//  SECTION 10: MAIN LOOP
// ============================================================
void loop() {
  Blynk.run();
  io.run();

  // Take samples and compute everything
  takeSamples();
  computePowerFactor();
  computeMeasurements();
  computeWaveformMetrics();
  detectLoadType();
  detectInrush();
  updateEnergy();
  updateExtremes();
  applyPFCorrectionLogic();

  // Blynk: update every 1 second
  // Adafruit IO: update every 3 seconds (prevent feed flooding)
  unsigned long now = millis();
  if (now - lastBlynkUpdate >= 1000) {
    updateBlynk();
    lastBlynkUpdate = now;
  }
  if (now - lastAdafruitUpdate >= 3000) {
    updateAdafruitIO();
    lastAdafruitUpdate = now;
  }
}

// ============================================================
//  SECTION 11: SAMPLING
// ============================================================
void takeSamples() {
  vZeroFound = false;
  cZeroFound = false;
  voltagePeak = 0.0f;
  currentPeak = 0.0f;

  for (int i = 0; i < SAMPLES; i++) {
    // Read ADC and convert to physical values
    float vRaw = ((float)analogRead(VOLTAGE_PIN) / ADC_MAX) * ADC_VREF;
    float iRaw = ((float)analogRead(CURRENT_PIN) / ADC_MAX) * ADC_VREF;

    voltageSamples[i] = (vRaw - ZMPT_VOFFSET) * ZMPT_SCALE;
    currentSamples[i] = (iRaw - ACS_VOFFSET) / ACS_SENSITIVITY;

    // Track peaks
    float absV = fabs(voltageSamples[i]);
    float absC = fabs(currentSamples[i]);
    if (absV > voltagePeak) voltagePeak = absV;
    if (absC > currentPeak) currentPeak = absC;

    // Zero crossing detection for phase shift
    // Record sample index of crossing for accurate phase calculation
    if (i > 0) {
      if (!vZeroFound && voltageSamples[i] > 0 && voltageSamples[i-1] < 0) {
        vZeroTime  = i;   // store sample index
        vZeroFound = true;
      }
      if (!cZeroFound && currentSamples[i] > 0 && currentSamples[i-1] < 0) {
        cZeroTime  = i;   // store sample index
        cZeroFound = true;
      }
    }

    // Maintain ~10kHz sampling rate
    unsigned long t = micros();
    while (micros() - t < SAMPLE_INTERVAL);
  }
}

// ============================================================
//  SECTION 12: POWER FACTOR COMPUTATION (PHASE SHIFT METHOD)
// ============================================================
void computePowerFactor() {
  if (vZeroFound && cZeroFound) {
    // Phase shift in samples, convert to time using sample interval
    float deltaSamples = (float)((long)cZeroTime - (long)vZeroTime);
    float deltaTimeSec = deltaSamples * (SAMPLE_INTERVAL / 1e6f);
    float phi          = 2.0f * PI * MAINS_FREQ * fabs(deltaTimeSec);
    powerFactor        = cos(phi);
    powerFactor        = constrain(powerFactor, 0.0f, 1.0f);
  } else {
    powerFactor = 0.0f;
  }
}

// ============================================================
//  SECTION 13: MEASUREMENTS
// ============================================================
void computeMeasurements() {
  // RMS
  voltage = calculateRMS(voltageSamples, SAMPLES);
  current = calculateRMS(currentSamples, SAMPLES);

  // DC components
  voltageDC = calculateMean(voltageSamples, SAMPLES);
  currentDC = calculateMean(currentSamples, SAMPLES);

  // Power
  apparentPower = voltage * current;
  realPower     = apparentPower * powerFactor;

  float phi     = acos(constrain(powerFactor, -1.0f, 1.0f));
  reactivePower = apparentPower * sin(phi);

  // Complex power string
  complexPower = String(realPower, 2) + " + j" + String(reactivePower, 2);

  // Impedance components — must be calculated BEFORE building the string
  if (current > MIN_CURRENT) {
    resistance = realPower     / (current * current);
    reactance  = reactivePower / (current * current);
  } else {
    resistance = 0.0f;
    reactance  = 0.0f;
  }
  complexImpedance = String(resistance, 2) + " + j" + String(reactance, 2);

  // PF label
  if (reactivePower > 0.1f)       pfLabel = String(powerFactor, 2) + " lagging";
  else if (reactivePower < -0.1f) pfLabel = String(powerFactor, 2) + " leading";
  else                            pfLabel = String(powerFactor, 2) + " (unity)";

  // Chip temperature
  // (ESP32 internal temp sensor)
  // Note: temperatureRead() returns Fahrenheit on some cores
  // Adjust formula if needed
}

// ============================================================
//  SECTION 14: WAVEFORM QUALITY METRICS
// ============================================================
void computeWaveformMetrics() {
  float meanV = calculateMean(voltageSamples, SAMPLES);
  float meanC = calculateMean(currentSamples, SAMPLES);

  stddevV = calculateStdDev(voltageSamples, SAMPLES, meanV);
  stddevC = calculateStdDev(currentSamples, SAMPLES, meanC);

  // Crest factor = Peak / RMS
  crestFactorV = (voltage > 0.1f) ? voltagePeak / voltage : 0.0f;
  crestFactorC = (current > 0.01f) ? currentPeak / current : 0.0f;

  // Form factor = RMS / Mean of absolute values
  float sumAbsV = 0.0f, sumAbsC = 0.0f;
  for (int i = 0; i < SAMPLES; i++) {
    sumAbsV += fabs(voltageSamples[i]);
    sumAbsC += fabs(currentSamples[i]);
  }
  float meanAbsV = sumAbsV / SAMPLES;
  float meanAbsC = sumAbsC / SAMPLES;

  formFactorV = (meanAbsV > 0.1f) ? voltage / meanAbsV : 0.0f;
  formFactorC = (meanAbsC > 0.01f) ? current / meanAbsC : 0.0f;
}

// ============================================================
//  SECTION 15: LOAD TYPE DETECTION
// ============================================================
void detectLoadType() {
  if (current < MIN_CURRENT) {
    loadType = "No Load";
    return;
  }
  // Crest factor for pure sine = 1.414
  // Higher crest = switching/non-linear load
  // Form factor for pure sine = 1.11
  if (crestFactorC > 2.0f) {
    loadType = "Non-linear / Switching";
  } else if (reactance > 5.0f && reactivePower > 0.1f) {
    loadType = "Inductive (Motor/Coil)";
  } else if (reactance < -5.0f && reactivePower < -0.1f) {
    loadType = "Capacitive";
  } else if (powerFactor > 0.95f) {
    loadType = "Resistive (Heater/Lamp)";
  } else {
    loadType = "Mixed";
  }
}

// ============================================================
//  SECTION 16: INRUSH DETECTION
// ============================================================
void detectInrush() {
  float deltaCurrent = current - previousCurrent;

  // Inrush ratio: how much current spiked relative to steady state
  if (previousCurrent > MIN_CURRENT) {
    inrushRatio = fabs(deltaCurrent) / previousCurrent;
  } else {
    inrushRatio = fabs(deltaCurrent);
  }

  // Log inrush event if significant — with cooldown to prevent log spam
  static unsigned long lastInrushEvent = 0;
  unsigned long nowMs2 = millis();
  if (inrushRatio > 2.0f && fabs(deltaCurrent) > 0.5f && (nowMs2 - lastInrushEvent > 3000)) {
    Blynk.logEvent("inrush_detected", "Inrush! Ratio: " + String(inrushRatio, 2)
                   + " ΔI=" + String(deltaCurrent, 2) + "A");
    lastInrushEvent = nowMs2;
  }

  // Track max inrush
  if (inrushRatio > maxInrushRatio) {
    maxInrushRatio = inrushRatio;
    tsMaxInrush    = getUnixTime();
    saveAdvancedToNVS();
    Blynk.virtualWrite(V_MAX_INRUSH, maxInrushRatio);
    Blynk.virtualWrite(V_TS_INRUSH, getFormattedTime(tsMaxInrush));
  }

  // Load switched event — with cooldown to prevent log spam
  static unsigned long lastLoadEvent = 0;
  unsigned long nowMs = millis();
  if (fabs(deltaCurrent) > 0.3f && (nowMs - lastLoadEvent > 2000)) {
    String dir = deltaCurrent > 0 ? "ON" : "OFF";
    Blynk.logEvent("load_event", "Load " + dir + " ΔI=" + String(deltaCurrent, 2) + "A");
    lastLoadEvent = nowMs;
  }

  previousCurrent = current;
}

// ============================================================
//  SECTION 17: ENERGY ACCUMULATION & MONTHLY ESTIMATE
// ============================================================
void updateEnergy() {
  unsigned long nowMs       = millis();
  float deltaHours          = (nowMs - lastEnergyUpdate) / 3600000.0f;
  energyKWh                += (realPower / 1000.0f) * deltaHours;
  lastEnergyUpdate          = nowMs;

  // Save energy to NVS every 60 seconds
  static unsigned long lastEnergySave = 0;
  if (nowMs - lastEnergySave > 60000) {
    preferences.begin("energyData", false);
    preferences.putFloat("energyKWh", energyKWh);
    preferences.end();
    lastEnergySave = nowMs;
  }

  // Monthly estimate based on running average since last reset
  float elapsedHours = (nowMs - energyStartMs) / 3600000.0f;
  if (elapsedHours > 0.1f) {
    float avgPowerKW       = energyKWh / elapsedHours;
    estimatedMonthlyEnergy = roundf(avgPowerKW * 730.5f * 1000.0f) / 1000.0f;
  }
}

// ============================================================
//  SECTION 18: EXTREMES TRACKING
// ============================================================
void updateExtremes() {
  bool updated = false;
  unsigned long t = getUnixTime();

  // Snapshot volatile variable to avoid race condition with ISR
  float freqSnapshot = measuredFrequency;

  if (voltage < minVoltage) { minVoltage = voltage; tsMinV = t; updated = true; }
  if (voltage > maxVoltage) { maxVoltage = voltage; tsMaxV = t; updated = true; }
  if (current < minCurrent) { minCurrent = current; tsMinC = t; updated = true; }
  if (current > maxCurrent) { maxCurrent = current; tsMaxC = t; updated = true; }
  if (freqSnapshot < minFrequency) { minFrequency = freqSnapshot; tsMinF = t; updated = true; }
  if (freqSnapshot > maxFrequency) { maxFrequency = freqSnapshot; tsMaxF = t; updated = true; }
  if (powerFactor < minPF) { minPF = powerFactor; tsMinPF = t; updated = true; }
  if (powerFactor > maxPF) { maxPF = powerFactor; tsMaxPF = t; updated = true; }

  if (updated) saveExtremesToNVS();
}

// ============================================================
//  SECTION 19: PF CORRECTION LOGIC (ZVS)
// ============================================================
void applyPFCorrectionLogic() {
  if (current < MIN_CURRENT) {
    // No load — remove all caps at next zero cross
    if (currentCapacitanceUF > 0) {
      pendingCapRemove = true;
    }
    pfBadCycleCount = 0;
    return;
  }

  bool pfIsGood = (powerFactor >= PF_DEADBAND_LOW);

  if (pfIsGood) {
    pfBadCycleCount = 0;
    // If caps are connected and PF is now good, remove at next zero cross
    if (currentCapacitanceUF > 0 && !pendingCapRemove) {
      pendingCapRemove = true;
      pendingCapSwitch = false;
    }
    return;
  }

  // PF is bad
  pfBadCycleCount++;

  if (pfBadCycleCount >= PF_BAD_CYCLES) {
    pfBadCycleCount = PF_BAD_CYCLES;  // cap it

    // Calculate required capacitance
    int targetUF = calculateRequiredCapacitance(powerFactor, voltage, current);
    targetUF     = constrain(targetUF, 0, 36);

    if (targetUF != currentCapacitanceUF && !pendingCapSwitch) {
      pendingCapTarget = targetUF;
      pendingCapSwitch = true;   // ISR will apply at next zero cross
      pendingCapRemove = false;

      // Log correction event — cooldown to prevent spam
      static unsigned long lastPFCEvent = 0;
      unsigned long nowPFC = millis();
      applyCorrectionFlag = true;
      pfCorrectionCount++;
      tsLastPFC = getUnixTime();
      saveAdvancedToNVS();
      Blynk.virtualWrite(V_PFC_COUNT, pfCorrectionCount);
      Blynk.virtualWrite(V_TS_PFC, getFormattedTime(tsLastPFC));
      if (nowPFC - lastPFCEvent > 5000) {
        Blynk.logEvent("pf_correction", "PF=" + String(powerFactor, 2)
                       + " Applying " + String(targetUF) + "uF");
        lastPFCEvent = nowPFC;
      }
      Blynk.virtualWrite(V_TARGET_CAP, targetUF);
      applyCorrectionFlag = false;
    }
  }
}

// ============================================================
//  SECTION 20: CAPACITANCE CALCULATION
// ============================================================
// Calculates required capacitance (µF) to correct PF to ~1.0
// Formula: C = Q / (2π f V²) where Q = P(tan φ1 - tan φ2)
// Target φ2 = 0 (unity PF), so tan φ2 = 0
int calculateRequiredCapacitance(float pf, float v, float i) {
  if (pf <= 0.0f || pf >= 1.0f || v < 10.0f || i < MIN_CURRENT) return 0;

  float phi1    = acos(pf);
  float tanPhi1 = tan(phi1);
  float P       = v * i * pf;                        // Real power in W
  float Q       = P * tanPhi1;                       // Reactive power to compensate
  float C_farads= Q / (2.0f * PI * MAINS_FREQ * v * v); // in Farads
  float C_uF    = C_farads * 1e6f;                   // convert to µF

  return (int)roundf(C_uF);
}

// Capacitor pin array in IRAM for safe ISR access
static const int IRAM_ATTR capPinsISR[NUM_CAPS] = {
  CAP_PIN_C1, CAP_PIN_C2, CAP_PIN_C3,
  CAP_PIN_C4, CAP_PIN_C5, CAP_PIN_C6, CAP_PIN_C7
};

// ============================================================
//  SECTION 21: APPLY CAPACITOR BITMASK
// (Called from ISR at zero crossing — keep it fast!)
// ============================================================
void IRAM_ATTR applyCapacitorMask(uint8_t mask) {
  for (int i = 0; i < NUM_CAPS; i++) {
    digitalWrite(capPinsISR[i], (mask >> i) & 0x01 ? HIGH : LOW);
  }
}

// ============================================================
//  SECTION 22: BLYNK UPDATE
// ============================================================
void updateBlynk() {
  if (!Blynk.connected()) return;

  // Group 1: Core measurements
  Blynk.virtualWrite(V_VOLTAGE,    voltage);
  Blynk.virtualWrite(V_CURRENT,    current);
  Blynk.virtualWrite(V_POWER,      realPower);
  Blynk.virtualWrite(V_APPARENT,   apparentPower);
  Blynk.virtualWrite(V_REACTIVE,   reactivePower);
  Blynk.virtualWrite(V_PF,         powerFactor);
  Blynk.virtualWrite(V_PF_LABEL,   pfLabel);
  Blynk.virtualWrite(V_FREQ,       measuredFrequency);
  yield();

  // Group 2: Energy
  Blynk.virtualWrite(V_ENERGY,     energyKWh);
  Blynk.virtualWrite(V_MONTHLY,    estimatedMonthlyEnergy);
  Blynk.virtualWrite(V_DC_VOLT,    voltageDC);
  Blynk.virtualWrite(V_DC_CURR,    currentDC);
  yield();

  // Group 3: Waveform metrics
  Blynk.virtualWrite(V_CREST_V,    crestFactorV);
  Blynk.virtualWrite(V_CREST_C,    crestFactorC);
  Blynk.virtualWrite(V_FORM_V,     formFactorV);
  Blynk.virtualWrite(V_FORM_C,     formFactorC);
  Blynk.virtualWrite(V_STDDEV_V,   stddevV);
  Blynk.virtualWrite(V_STDDEV_C,   stddevC);
  yield();

  // Group 4: Load info & impedance
  Blynk.virtualWrite(V_LOAD_TYPE,  loadType);
  Blynk.virtualWrite(V_INRUSH,     inrushRatio);
  Blynk.virtualWrite(V_COMPLEX_S,  complexPower);
  Blynk.virtualWrite(V_COMPLEX_Z,  complexImpedance);
  Blynk.virtualWrite(V_RESISTANCE, resistance);
  Blynk.virtualWrite(V_REACTANCE,  reactance);
  yield();

  // Group 5: Extremes
  Blynk.virtualWrite(V_MIN_VOLT,   minVoltage);
  Blynk.virtualWrite(V_MAX_VOLT,   maxVoltage);
  Blynk.virtualWrite(V_MIN_CURR,   minCurrent);
  Blynk.virtualWrite(V_MAX_CURR,   maxCurrent);
  Blynk.virtualWrite(V_MIN_FREQ,   minFrequency);
  Blynk.virtualWrite(V_MAX_FREQ,   maxFrequency);
  Blynk.virtualWrite(V_MIN_PF,     minPF);
  Blynk.virtualWrite(V_MAX_PF,     maxPF);
  yield();

  // Group 6: Extreme timestamps
  Blynk.virtualWrite(V_TS_MIN_V,   getFormattedTime(tsMinV));
  Blynk.virtualWrite(V_TS_MAX_V,   getFormattedTime(tsMaxV));
  Blynk.virtualWrite(V_TS_MIN_C,   getFormattedTime(tsMinC));
  Blynk.virtualWrite(V_TS_MAX_C,   getFormattedTime(tsMaxC));
  Blynk.virtualWrite(V_TS_MIN_F,   getFormattedTime(tsMinF));
  Blynk.virtualWrite(V_TS_MAX_F,   getFormattedTime(tsMaxF));
  Blynk.virtualWrite(V_TS_MIN_PF,  getFormattedTime(tsMinPF));
  Blynk.virtualWrite(V_TS_MAX_PF,  getFormattedTime(tsMaxPF));
  yield();

  // Group 7: Advanced stats
  Blynk.virtualWrite(V_MAX_INRUSH, maxInrushRatio);
  Blynk.virtualWrite(V_PFC_COUNT,  pfCorrectionCount);
  Blynk.virtualWrite(V_TS_INRUSH,  getFormattedTime(tsMaxInrush));
  Blynk.virtualWrite(V_TS_PFC,     getFormattedTime(tsLastPFC));
  Blynk.virtualWrite(V_TARGET_CAP, currentCapacitanceUF);
  yield();

  // Group 8: Cap bank relay states
  for (int i = 0; i < NUM_CAPS; i++) {
    Blynk.virtualWrite(V3 + i, digitalRead(capPins[i]));
  }

  // Chip temperature
  float chipTemp = temperatureRead();
  chipTemp = (chipTemp - 32.0f) * 5.0f / 9.0f;
  Blynk.virtualWrite(V_CHIP_TEMP, chipTemp);
}

// ============================================================
//  SECTION 23: ADAFRUIT IO UPDATE
// ============================================================
void updateAdafruitIO() {
  if (io.status() < AIO_CONNECTED) return;

  voltFeed->save(voltage);
  currFeed->save(current);
  powerFeed->save(realPower);
  pfFeed->save(powerFactor);
  freqFeed->save(measuredFrequency);
  energyFeed->save(energyKWh);
  dcVoltageFeed->save(voltageDC);
  dcCurrentFeed->save(currentDC);
  crestVoltageFeed->save(crestFactorV);
  crestCurrentFeed->save(crestFactorC);
  formVoltageFeed->save(formFactorV);
  formCurrentFeed->save(formFactorC);
  stddevVoltageFeed->save(stddevV);
  stddevCurrentFeed->save(stddevC);
  loadTypeFeed->save(loadType);
  inrushRatioFeed->save(inrushRatio);
  maxInrushFeed->save(maxInrushRatio);
  pfCorrCountFeed->save(pfCorrectionCount);
  monthlyEstFeed->save(estimatedMonthlyEnergy);

  float chipTemp = temperatureRead();
  chipTemp = (chipTemp - 32.0f) * 5.0f / 9.0f;
  chipTempFeed->save(chipTemp);
}

// ============================================================
//  SECTION 24: BLYNK WRITE HANDLERS
// ============================================================

// Reset energy
BLYNK_WRITE(V10) {
  if (param.asInt() == 1) {
    energyKWh    = 0.0f;
    energyStartMs = millis();
    preferences.begin("energyData", false);
    preferences.putFloat("energyKWh", 0.0f);
    preferences.end();
    Blynk.virtualWrite(V_ENERGY,  0.0f);
    Blynk.virtualWrite(V_MONTHLY, 0.0f);
    Blynk.virtualWrite(V10, 0);
  }
}

// Reset all extremes
BLYNK_WRITE(V34) {
  if (param.asInt() == 1) {
    minVoltage = 9999.0f; maxVoltage = 0.0f;
    minCurrent = 9999.0f; maxCurrent = 0.0f;
    minFrequency = 9999.0f; maxFrequency = 0.0f;
    minPF = 1.0f; maxPF = 0.0f;
    tsMinV = tsMaxV = tsMinC = tsMaxC = 0;
    tsMinF = tsMaxF = tsMinPF = tsMaxPF = 0;
    saveExtremesToNVS();

    Blynk.virtualWrite(V_MIN_VOLT,  minVoltage);
    Blynk.virtualWrite(V_MAX_VOLT,  maxVoltage);
    Blynk.virtualWrite(V_MIN_CURR,  minCurrent);
    Blynk.virtualWrite(V_MAX_CURR,  maxCurrent);
    Blynk.virtualWrite(V_MIN_FREQ,  minFrequency);
    Blynk.virtualWrite(V_MAX_FREQ,  maxFrequency);
    Blynk.virtualWrite(V_MIN_PF,    minPF);
    Blynk.virtualWrite(V_MAX_PF,    maxPF);
    Blynk.virtualWrite(V_TS_MIN_V,  "N/A");
    Blynk.virtualWrite(V_TS_MAX_V,  "N/A");
    Blynk.virtualWrite(V_TS_MIN_C,  "N/A");
    Blynk.virtualWrite(V_TS_MAX_C,  "N/A");
    Blynk.virtualWrite(V_TS_MIN_F,  "N/A");
    Blynk.virtualWrite(V_TS_MAX_F,  "N/A");
    Blynk.virtualWrite(V_TS_MIN_PF, "N/A");
    Blynk.virtualWrite(V_TS_MAX_PF, "N/A");
    Blynk.virtualWrite(V34, 0);
  }
}

// Reset max inrush
BLYNK_WRITE(V57) {
  if (param.asInt() == 1) {
    maxInrushRatio = 0.0f;
    tsMaxInrush    = 0;
    preferences.begin("advanced", false);
    preferences.putFloat("maxInrush", 0.0f);
    preferences.putULong("tsInrush",  0);
    preferences.end();
    Blynk.virtualWrite(V_MAX_INRUSH, 0.0f);
    Blynk.virtualWrite(V_TS_INRUSH,  "N/A");
    Blynk.virtualWrite(V57, 0);
  }
}

// Reset PF correction count
BLYNK_WRITE(V58) {
  if (param.asInt() == 1) {
    pfCorrectionCount = 0;
    tsLastPFC         = 0;
    preferences.begin("advanced", false);
    preferences.putInt("pfCount", 0);
    preferences.putULong("tsPFC",  0);
    preferences.end();
    Blynk.virtualWrite(V_PFC_COUNT, 0);
    Blynk.virtualWrite(V_TS_PFC,    "N/A");
    Blynk.virtualWrite(V58, 0);
  }
}

// ============================================================
//  SECTION 25: NVS SAVE & LOAD
// ============================================================
void saveExtremesToNVS() {
  preferences.begin("extremes", false);
  preferences.putFloat("minV",    minVoltage);
  preferences.putFloat("maxV",    maxVoltage);
  preferences.putFloat("minC",    minCurrent);
  preferences.putFloat("maxC",    maxCurrent);
  preferences.putFloat("minF",    minFrequency);
  preferences.putFloat("maxF",    maxFrequency);
  preferences.putFloat("minPF",   minPF);
  preferences.putFloat("maxPF",   maxPF);
  preferences.putULong("tsMinV",  tsMinV);
  preferences.putULong("tsMaxV",  tsMaxV);
  preferences.putULong("tsMinC",  tsMinC);
  preferences.putULong("tsMaxC",  tsMaxC);
  preferences.putULong("tsMinF",  tsMinF);
  preferences.putULong("tsMaxF",  tsMaxF);
  preferences.putULong("tsMinPF", tsMinPF);
  preferences.putULong("tsMaxPF", tsMaxPF);
  preferences.end();
}

void saveAdvancedToNVS() {
  preferences.begin("advanced", false);
  preferences.putFloat("maxInrush", maxInrushRatio);
  preferences.putULong("tsInrush",  tsMaxInrush);
  preferences.putInt("pfCount",     pfCorrectionCount);
  preferences.putULong("tsPFC",     tsLastPFC);
  preferences.end();
}

void loadFromNVS() {
  preferences.begin("extremes", true);
  minVoltage   = preferences.getFloat("minV",    9999.0f);
  maxVoltage   = preferences.getFloat("maxV",    0.0f);
  minCurrent   = preferences.getFloat("minC",    9999.0f);
  maxCurrent   = preferences.getFloat("maxC",    0.0f);
  minFrequency = preferences.getFloat("minF",    9999.0f);
  maxFrequency = preferences.getFloat("maxF",    0.0f);
  minPF        = preferences.getFloat("minPF",   1.0f);
  maxPF        = preferences.getFloat("maxPF",   0.0f);
  tsMinV       = preferences.getULong("tsMinV",  0);
  tsMaxV       = preferences.getULong("tsMaxV",  0);
  tsMinC       = preferences.getULong("tsMinC",  0);
  tsMaxC       = preferences.getULong("tsMaxC",  0);
  tsMinF       = preferences.getULong("tsMinF",  0);
  tsMaxF       = preferences.getULong("tsMaxF",  0);
  tsMinPF      = preferences.getULong("tsMinPF", 0);
  tsMaxPF      = preferences.getULong("tsMaxPF", 0);
  preferences.end();

  preferences.begin("advanced", true);
  maxInrushRatio    = preferences.getFloat("maxInrush", 0.0f);
  tsMaxInrush       = preferences.getULong("tsInrush",  0);
  pfCorrectionCount = preferences.getInt("pfCount",     0);
  tsLastPFC         = preferences.getULong("tsPFC",     0);
  preferences.end();

  preferences.begin("energyData", true);
  energyKWh = preferences.getFloat("energyKWh", 0.0f);
  preferences.end();
}

// ============================================================
//  SECTION 26: HELPER FUNCTIONS
// ============================================================
float calculateRMS(float* samples, int count) {
  float sum = 0.0f;
  for (int i = 0; i < count; i++) sum += samples[i] * samples[i];
  return sqrt(sum / count);
}

float calculateMean(float* samples, int count) {
  float sum = 0.0f;
  for (int i = 0; i < count; i++) sum += samples[i];
  return sum / count;
}

float calculateStdDev(float* samples, int count, float mean) {
  float sum = 0.0f;
  for (int i = 0; i < count; i++) {
    float d = samples[i] - mean;
    sum += d * d;
  }
  return sqrt(sum / count);
}

unsigned long getUnixTime() {
  time_t t;
  time(&t);
  return (unsigned long)t;
}

String getFormattedTime(unsigned long unixTime) {
  if (unixTime == 0) return "N/A";
  time_t t = (time_t)unixTime;
  struct tm *ti = localtime(&t);
  char buf[32];
  strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", ti);
  return String(buf);
}
