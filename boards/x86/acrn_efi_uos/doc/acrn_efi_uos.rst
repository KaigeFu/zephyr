.. _acrn_efi_uos:

ACRN/EFI User OS
################

Overview
********

This board configuration enables kernel support for running Zephyr as
User OS under the ACRN Hypervisor booted under UEFI.

.. note::
   This board configuration has only been verified to work on
   the UP Squared board when booting ACRN under UEFI. Running on ACRN
   booted with SBL is not supported with this board configuration.

Hardware
********

As ACRN provides a virtualized environment for Zephyr, hardware support
limits to what are provided by ACRN.

Supported Features
==================

This board supports the following hardware features:

* LOAPIC Timer

* Advanced Programmed Interrupt Controller (APIC)

* Serial Ports in Polling and Interrupt Driven Modes

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| LOAPIC    | on-chip    | system clock          |
| Timer     |            |                       |
+-----------+------------+-----------------------+
| APIC      | on-chip    | interrupt controller  |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+

Serial Port Polling Mode Support
--------------------------------

The polling mode serial port allows debug output to be printed.

Serial Port Interrupt Mode Support
----------------------------------

The interrupt mode serial port provides general serial communication
and external communication.

Interrupt Controller
--------------------

This board uses the kernel's static Interrupt Descriptor Table (IDT) to program the
Advanced Programmable Interrupt Controller (APIC) interrupt redirection table.

+-----+---------+--------------------------+
| IRQ | Remarks | Used by Zephyr Kernel    |
+=====+=========+==========================+
| 4   | UART_0  | serial port when used in |
|     |         | interrupt mode           |
+-----+---------+--------------------------+
| 5   | UART_1  | serial port when used in |
|     |         | interrupt mode           |
+-----+---------+--------------------------+

System Clock Support
--------------------

The board configuration uses LOAPIC Timer as a system clock timer.

Memory Mappings
===============

* Flash starts at address 0x10000000.
* Memory starts at address 0x40000000.

Booting
=======

TBD
