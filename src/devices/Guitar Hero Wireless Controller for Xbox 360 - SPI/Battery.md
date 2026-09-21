# Device battery information

On the Olimex ESP3-S3-LiPo by default it has a crap charge rate. 100mA with about 30 hours for a full charge!

This is so current charge never gets too much during charging + use.

We don't pull too much current though...

Advice exists on "change R4 to smaller resistor in with 0402 size and it will have higher current for charge. My advice is not to go as low as 2.2k, maybe down to 3.3k (which is 300mA of current for charging)."

## Physical battery
<table>
    <tr>
      <th colspan="2">Battery Information</th>
    </tr>
    <tr>
      <td>Battery Capacity</td>
      <td>3000mAh</td>
    </tr>
    <tr>
      <td>voltage</td>
      <td>⚡3.7v</td>
    </tr>
    <tr>
      <td>Charge Ending Voltage</td>
      <td>4.20 ± 0.03v</td>
    </tr>
    <tr>
      <td>Max Charging Current</td>
      <td>0.5A</td>
    </tr>
    <tr>
      <td>Max Discharging Current</td>
      <td>0.5A</td>
    </tr>
</table>

## Theoretical approx charge
<table>
  <tr>
    <th>Voltage</th>
    <th>Charge %</th>
  </tr>
  <tr>
    <td>4.20 V</td>
    <td>100%</td>
  </tr>
  <tr>
    <td>4.15 V</td>
    <td>≈90%</td>
  </tr>
  <tr>
    <td>4.10 V</td>
    <td>≈85%</td>
  </tr>
  <tr>
    <td>4.05 V</td>
    <td>≈80%</td>
  </tr>
  <tr>
    <td>4.00 V</td>
    <td>≈70–75%</td>
  </tr>
  <tr>
    <td>3.90 V</td>
    <td>≈55–60%</td>
  </tr>
  <tr>
    <td>3.80 V</td>
    <td>≈40–45%</td>
  </tr>
  <tr>
    <td>3.70 V</td>
    <td>≈25–30%</td>
  </tr>
  <tr>
    <td>3.60 V</td>
    <td>≈10–15%</td>
  </tr>
  <tr>
    <td>3.50 V</td>
    <td>≈5–8%</td>
  </tr>
  <tr>
    <td>3.40 V</td>
    <td>≈2–3%</td>
  </tr>
  <tr>
    <td>3.30 V</td>
    <td>≈1%</td>
  </tr>
  <tr>
    <td>3.20 V</td>
    <td>≈0% (practically empty)</td>
  </tr>
  <tr>
    <td>3.00 V</td>
    <td>0% (deep‑discharge territory)</td>
  </tr>
</table>

## Measured readings

⚡- Voltage on battery (multimeter reading)
🔋- Raw reading of battery monitor pin
🔌- Raw reading of (USB) power monitor pin
⛽ - Charging state (so readings could rise while taking them)

<table style="white-space: nowrap;"> 
    <tr>
        <th>Phase</th>
        <th colspan="2">Battery Only</th>
        <th colspan="2">USB + Normal</th>
        <th colspan="2">USB + Boot Screen</th>
    </tr>
    <tr>
        <th></th>
        <th>Off<br>No Load</th>
        <th>On</th>
        <th>No Battery</th>
        <th>Battery<br>(Charging)</th>
        <th>No Battery</th>
        <th>Battery<br>(Charging)</th>
    </tr>
    <tr>
        <td>Low Power<br>LED flicker<br>Crashing</td>
        <td>⚡3.133v</td>
        <td>⚡3.05v<br>🔋1572<br>🔌0</td>
        <td>⚡3.124v<br>🔋1902<br>🔌4095</td>
        <td>⚡3.156v ⛽<br>🔋1434<br>🔌3951</td>
        <td>⚡3.130v<br>🔋1886<br>🔌4095</td>
        <td>⚡3.164v ⛽<br>🔋1425<br>🔌3983</td>
    </tr>
    <tr>
        <td>Measure 2</td>
        <td>⚡3.483v</td>
        <td>⚡3.433v<br>🔋1816<br>🔌395</td>
        <td>⚡3.472v<br>🔋1862<br>🔌4095</td>
        <td>⚡3.496v ⛽<br>🔋1695<br>🔌3928</td>
        <td>⚡3.475v<br>🔋1865<br>🔌4095</td>
        <td>⚡3.501v ⛽<br>🔋1731<br>🔌4028</td>
    </tr>
    <tr>
        <td>Measure 3</td>
        <td>⚡3.650v</td>
        <td>⚡3.615v<br>🔋1902<br>🔌0</td>
        <td>⚡3.643v<br>🔋1885<br>🔌4095</td>
        <td>⚡3.662v ⛽<br>🔋1826<br>🔌3974</td>
        <td>⚡3.646v<br>🔋1915<br>🔌4095</td>
        <td>⚡3.665v ⛽<br>🔋1860<br>🔌4089</td>
    </tr>
    <tr>
        <td>Measure 4</td>
        <td>⚡3.724v</td>
        <td>⚡3.691v<br>🔋1980<br>🔌0</td>
        <td>⚡3.714v<br>🔋1895<br>🔌4095</td>
        <td>⚡3.732v ⛽<br>🔋1867<br>🔌3988</td>
        <td>⚡3.717v<br>🔋1916<br>🔌4095</td>
        <td>⚡3.735v ⛽<br>🔋1848<br>🔌4039</td>
    </tr>
    <tr>
        <td>Measure 5</td>
        <td>⚡3.778v</td>
        <td>⚡3.741v<br>🔋1992<br>🔌0</td>
        <td>⚡3.770v<br>🔋1877<br>🔌4095</td>
        <td>⚡3.787v ⛽<br>🔋1936<br>🔌4005</td>
        <td>⚡3.774v<br>🔋1903<br>🔌4095</td>
        <td>⚡3.792v ⛽<br>🔋1870<br>🔌4093</td>
    </tr>
    <tr>
        <td>Measure 6</td>
        <td>⚡3.844v</td>
        <td>⚡3.809v<br>🔋2030<br>🔌0</td>
        <td>⚡3.833v<br>🔋1888<br>🔌4095</td>
        <td>⚡3.848v ⛽<br>🔋1946<br>🔌4010</td>
        <td>⚡3.836v<br>🔋1913<br>🔌4095</td>
        <td>⚡3.854v ⛽<br>🔋1926<br>🔌4093 ⚠️4095 (Numerous)</td>
    </tr>
    <tr>
        <td>Measure 7</td>
        <td>⚡3.884v</td>
        <td>⚡3.849v<br>🔋2032<br>🔌0</td>
        <td>⚡3.872v<br>🔋1886<br>⚠️4095</td>
        <td>⚡3.886v ⛽<br>🔋1962<br>🔌4027</td>
        <td>⚡3.876v<br>🔋1915<br>🔌4095</td>
        <td>⚡3.889v ⛽<br>🔋1954<br>🔌4088 ⚠️4095 (Numerous)</td>
    </tr>
    <tr>
        <td>Measure 8</td>
        <td>⚡3.996v</td>
        <td>⚡3.961v<br>🔋2075<br>🔌0</td>
        <td>⚡3.987v<br>🔋1879<br>🔌4087</td>
        <td>⚡3.998v ⛽<br>🔋1977<br>🔌3983</td>
        <td>⚡3.989v<br>🔋1915<br>🔌4095</td>
        <td>⚡3.997v ⛽<br>🔋1980<br>⚠️4095 (All)</td>
    </tr> 
    <tr>
        <td colspan="99">Charging becomes very slow at 4.0. Note that battery life much better at 80% charge so recommend we don't go above </td>
    </tr>
    <tr>
        <td>Measure 9</td>
        <td>⚡3.995v</td>
        <td>⚡3.964v<br>🔋2147<br>🔌0</td>
        <td>⚡3.989v<br>🔋1876<br>🔌4085</td>
        <td>⚡3.996v ⛽<br>🔋1983<br>🔌3997</td>
        <td>⚡3.991v<br>🔋1890<br>🔌4095</td>
        <td>⚡4.000v ⛽<br>🔋1979<br>🔌3987 (All)</td>
    </tr> 
</table>

## Flutter

Steady enough voltage, wavering readings
<table style="white-space: nowrap;">
    <tr>
        <th colspan="2">3.748v</th>
    </tr>
    <tr>
        <th>Battery</th>
        <th>Power</th>
    </tr>
    <tr>
        <td>1856</td>
        <td>4059</td>
    </tr>
    <tr>
        <td>1856</td>
        <td>3996</td>
    </tr>
    <tr>
        <td>1851</td>
        <td>4043</td>
    </tr>
    <tr>
        <td>1848</td>
        <td>4039</td>
    </tr>
    <tr>
        <td>0.43% 8 Range</td>
        <td>1.56% 63 Range</td>
    <tr>
        <th colspan="2">USB + No Battery (stable voltage)</th>
    </tr>
    <tr>
        <th>Battery</th>
        <th>Power</th>
    </tr>
    <tr>
        <td>1915</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1916</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1912</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1916</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1914</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1913</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1906</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1912</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>0.52% 10 Range</td>
        <td>0% 0 Range</td>
    <tr>
    <tr>
        <th colspan="2">USB + Battery (charging)</th>
    </tr>
    <tr>
        <th>Battery</th>
        <th>Power</th>
    </tr>
    <tr>
        <td>1836</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1843</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1842</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>1839</td>
        <td>4087</td>
    </tr>
    <tr>
        <td>1841</td>
        <td>4087</td>
    </tr>
    <tr>
        <td>1847</td>
        <td>4095</td>
    </tr>
    <tr>
        <td>0.6% 11 Range</td>
        <td>0.2% 8 Range</td>
    <tr>
</table>

## Observations

- 🔋Battery only pin reading is higher at lower voltage than battery pin when is charging
- 🔌Power = 4095 when USB plugged in and no battery
  - i.e. battery reading if usb and no battery connected
- 🔌Power = 3951 and similar when USB plugged in and battery connected (charging)
- 🔌Power also showing as 4095 when USB plugged in and battery connected so looks like have to monitor for a while and try pick up if anything is < 4095 over time = USB + battery charging (not just cable plugged in and no battery)
- Power Minimum we can take as something low e.g. 256 - anything less than this is no power (could be noise on the pin so may not be 0)
- i.e.🔌
  - power > Power Minimum & < 4095 ➜ state = power + charging
  - power ≥ 4095 ➜ state = power only (no battery)
  - power = 0 when no USB plugged in ➜ state = battery only
  - Note: Occasional 4095 when charging - suspect need to average to double check or only change if state changes and remains changed within a time period
- Turning USB on when power super low causing reboot when on battery in main screen
- Battery pin has around a +/- 0.25% flutter on it, with a range of around 10 for same voltage reading
- Power pin (when on battery) has a bigger flutter but we aren't interested so much in exact value here so can ignore.
## Conclusions

3.3 volts is about where we can consider a safe bottom end - i.e. 0% or `empty'.

4.0 volts is what we consider full. Can go above that point safely but were into > 80% charge territory which degrades battery life, and charging is slower.

We have ~359 counts of usable range. Raw flutter is ~10 counts (~0.5%), giving ~36 stable levels if we only changed battery % when the readings move by 10. By smoothing (e.g. averaging 3 readings), we reduce the effective flutter, which lets us safely lower the update threshold and get closer to ~100 distinct battery levels. In practise, average by 8.

We extrapolate figures to give the following ranges - only important points of calculation are noted.

<table style="white-space: nowrap;"> 
    <tr>
        <th>Phase</th>
        <th colspan="2">Battery Only</th>
        <th colspan="2">USB + Normal</th>
        <th colspan="2">USB + Boot Screen</th>
    </tr>
    <tr>
        <th></th>
        <th>Off</th>
        <th>Battery Only</th>
        <th>USB Only</th>
        <th>Battery + USB</th>
    </tr>
    <tr>
        <td>Pretend 0%<br>3.300v</td>
        <td></td>
        <td>⚡3.3v<br>🔋1731<br>🔌< 512</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡3.37v ⛽<br>🔋1598<br>512 <🔌< 4095</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡3.33v ⛽<br>🔋1571<br>512 <🔌< 4095</td>
    </tr>
    <tr>
        <td>Pretend 100%<br>4.000v</td>
        <td></td>
        <td>⚡4.00v<br>🔋2090<br/>🔌< 512</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡4.00v ⛽<br>🔋1977<br>512 <🔌< 4095</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡4.00v ⛽<br>🔋1980<br>512 <🔌< 4095</td>
    </tr>
    <tr>
        <td>Theoretical 120% (Max)<br>4.200v</td>
        <td></td>
        <td>⚡4.20v<br>🔋2193<br/>🔌< 512</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡4.20v ⛽<br>🔋2097<br>512 <🔌< 4095</td>
        <td><br><br>🔌4095 = USB</td>
        <td>⚡4.20v ⛽<br>🔋2101<br>512 <🔌< 4095</td>
    </tr>
</table>

USB + Normal and USB + Boot Screen can be considered same, even with different loads.

Full 4.2v would be (on our pretend scale) 129%

Slapping a capacitor in the mix to smooth out power at the bottom end will of course help too.

## State

Problem where we check for power = 4095 on the power pin to know when we are running on power only. At higher charge points it can hit 4095 with battery charging too, so can't on it's own be used to identify power only. Luckily the battery pin shows a difference between power only/no battery, and power + charging battery. Accounted for below...

<table> 
    <tr style="white-space: nowrap;">
        <th></th>
        <th>Off</th>
        <th>Battery</th>
        <th>Battery Full</th>
        <th>Battery + Power<br>Charging</th>
        <th>Battery + Power<br>Full</th>
        <th>Power Only</th>    
    </tr>
    <tr style="white-space: nowrap;">
        <td>State</td>
        <td>Off</td>
        <td>Battery</td>
        <td>Battery Full</td>
        <td>Powered + Battery Charging</td>
        <td>Powered + Battery Full</td>
        <td>Powered</td>
    </tr>
    <tr style="white-space: nowrap;">
        <td>Rule</td>
        <td></td>
        <td>🔋< 2090<br>🔌< 512</td>
        <td>🔋>= 2090<br>🔌< 512</td>
        <td>(🔌 > 512 & 🔌 < 4095)<br>...or...<br>(🔌 == 4095 & 🔋 > 1920)</td>
        <td>🔋>= 2090<br>🔌 > 512</td>
        <td>🔋< 1920<br>🔌== 4095</td>
    </tr>
    <tr>
        <td>Notes</td>
        <td>Device off - nothing to see</td>
        <td>No power sensed, device physically on, must be battery</td>
        <td>Power sensed but not max, or power max but battery under certain level</td>
        <td>Battery at 100% and power sensed. < 4095 on power always showed when charging battery. 4095 also showed at higher charged points but that clashes with 4095 for power only no battery state. However in a 4095 charging state the battery showed as > 1920. Power only the battery pin showed around 1915. Bingo!</td>
        <td>Battery is full (into overcharge) and power is sensed</td>
        <td>Power is sensed, no battery is plugged in, though still get a reading on battery pin</td>
    </tr>
</table>

## Pseudocode

Hardcoded

```
if (power < 512)
    if (battery < 2090)
        state = Battery
    else
        state = Battery Full
else
    if (battery > 1920) and (power < 4095)
        state = Powered + Battery Charging
    else if (battery >= 2090) and (power > 512)
        state = Powered + Battery Full
    else if (battery <> 1920) and (power == 4095)
        state = Powered
    else
        state = Unknown
```

Final

```

// Empty battery equivalent - 0% full (can be less, but then considered undercharged)
BATTERY_MIN 1731
BATTERY_MIN_V 3.3
// Full battery equivalent - 100% full (approx 80% physically charged, can be more, but then considered overcharged)
BATTERY_FULL 2090
BATTERY_FULL_V 4.0
// Theoretical actual battery max - ~130% overcharged (physically fine, shortens life of battery)
BATTERY_MAX 2193
BATTERY_MAX_V 4.20

// Magic number where when power pin reads 4095 we can tell if the battery is connected or not. < this number = no battery, just power
BATTERY_CHECK 1920

// Readings when power plugged in and on battery
// 0%->100% results here map to 0%->100% equivalent of battery range
// so theoretically when plugging in and out a USB cable, code changes range it calculates the % battery charge as an equivalent to as if it was just the battery
CHARGING_MIN 1598
CHARGING_MIN_V 3.37
CHARGING_FULL 1977
CHARGING_FULL_V 4.0
CHARGING_MAX 2097
CHARGING_MAX_V 4.20

// Consider power to be present if power pin above this (allows for some noise on power pin when not powered)
POWER_PRESENT 512
POWER_MAX 4095

var min
var min_v
var max
var max_v

battery = readPin(battery)
power = readPin(power)

if (power < POWER_PRESENT)
    min = BATTERY_MIN
    min_v = BATTERY_MIN_V
    max = BATTERY_MAX
    max_v = BATTERY_MAX_V

    if (battery < BATTERY_FULL)
        state = Battery
    else
        state = Battery Full
else
    min = CHARGING_MIN
    min_v = CHARGING_MIN_V
    max = CHARGING_MAX
    max_v = CHARGING_MAX_V

    if (battery > BATTERY_CHECK) and (power < POWER_MAX)
        state = Powered + Battery Charging
    else if (battery >= BATTERY_FULL) and (power > POWER_PRESENT)
        state = Powered + Battery Full
    else if (battery < BATTERY_CHECK) and (power == POWER_MAX)
        state = Powered
    else
        state = Unknown

percentage = fmap(battery, min, max, 0, 100)
clamped_percentage = clamp(percentage, 0, 100)
voltage = fmap(battery, min, max, min_v, max_v)
clamped_voltage = clamp(voltage, min_v, max_v)

Processing the results...

when state == Powered
    Draw power cord
when state = unknown
    End of world!
when state = Powered + Battery Full
    Draw full battery + power cord
when state = Powered + Battery Charging
    Draw battery charging (with power cord)
        Dynamic battery size based on percentage
when state = battery full
    Draw full battery
when state = battery
    if (clamped_percentage = 0)
        draw empty battery / go into battery empty mode
    else if (clamped_percentage < 5%)
        Draw battery + empty warning
    else
        draw battery + Dynamic battery size based on clamped_percentage
    
else
    state = unknown - end of days!

if (percentage > clamped_percentage)
    draw extra overcharge +

```