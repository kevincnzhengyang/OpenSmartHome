
# Open Smart Home

Grant the customer (owner) all rights to manage devices constructing Smart Home Solution.

event drive loop

event bits should be Orthogonal

# build

``` bash

# IDF Component Registry
idf.py add-dependency "espressif/qrcode^0.1.0~2"

idf.py add-dependency "espressif/mdns^1.3.0"

idf.py add-dependency "espressif/button^3.2.0"

idf.py add-dependency "espressif/led_strip^2.5.3"

idf.py add-dependency "espressif/coap^4.3.4~1"

```


``` bash

avahi-browse -tpk -r _shnode._udp

#Status (+ or =)
#Network interface (enx00000000b000)
#IP type (4 or 6)
#Service name ("sybo")
#Service type ("Remote Disk Management")
#Domain ("local")

```
