# CRC16-CCITT

The protocol uses CRC16-CCITT with:

- width: 16
- polynomial: `0x1021` (reflected implementation polynomial `0x8408`)
- initial value: `0x0000`
- xor-out: `0x0000`
- input reflection: true
- result reflection: true

Golden vectors are stored in `shared/fixtures/crc16.json`.
