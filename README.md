# s32k312mini-evb-pruebas

Pruebas con la placa de evaluación S32K312MINI-EVB, también llamada FRDM Automotive.

# S32K312MINI-EVBOARD

Placa de desarrollo de NXP que incluye:
- Microcontrolador S32K312 172 HDQFP
- On board debug NXP K26
- NXP SBC FS26 para control de alimentación y consumo


# Características técnicas del S32K312

## Mapa de memoria

### Memoria de programa

Tiene en total 2 MB, divididos en dos bloques de 1024 KB
- Program flash Block 0: `0x00400000` a `0x004FFFFF`
- Program flash Block 1: `0x00500000` a `0x005FFFFE`

### Memoria de datos

Tiene 128 KB de memoria de datos:
- Data flash Block 2: `0x10000000` a `0x1003FFFF`


### Memoria RAM

Tiene en total 192 KB de memoria RAM:

- ITCM_0_BACKDOOR `0x11000000` 32 KB
- UTEST           `0x1B000000`  8 KB
- DTCM            `0x20000000` 64 KB
- SRAM            `0x20400000` 96 KB
- DTCM Backdoor   `0x21000000` 64 KB

ITCM + DTCM + SRAM = 32 + 64 + 96 = 192 KB

## Mapa de interrupciones

|  Dirección  | Vector | Interrupt Request |
|-------------|--------|-------------------|
| 0x0000_000C |    3   |     Hard Fault    |
| 0x0000_0010 |    4   |  MemManage Fault  |
| 0x0000_0014 |    5   |     Bus Fault     |
| 0x0000_0018 |    6   |    Usage Fault    |

