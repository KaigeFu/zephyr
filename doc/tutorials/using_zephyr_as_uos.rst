.. _using_zephyr_as_uos:

Using Zephyr as User OS
#######################

The tutorial describes the steps to run zephyr as User OS on ACRN hypervisor. We are using
Kaby Lake-based NUC model NUC7i5DNH and you can try other platform ACRN supported as well.

Introduction to Zephyr
**********************

The Zephyr is a scalable real-time operating-system (RTOS) supporting multiple hardware architecture, optimized for resource constrained devices, and built with safety and security in mind.

Steps for Using Zephyr as User OS
*********************************

#. Build Zephyr and grub2 boot loader image

   First of all, please follow the `Development Environment Setup on Linux 
   <https://docs.zephyrproject.org/latest/getting_started/installation_linux.html>`_ to 
   setup Zephyr development environment.

   Get the source code. As we the patches to enable Zephyr as ACRN UOS has not been
   upstreamed. So, we use my a forked repo to hold the patches here. We will replace
   it with `upstreamed repo <https://github.com/zephyrproject-rtos/zephyr.git>`_.

   .. code-block:: none

      $ git clone https://github.com/KaigeFu/zephyr.git -b zaag_v1.14.0-rc1

   Build Zephyr

   .. code-block:: none

      $ cd samples/subsys/shell/shell_module/
      $ mkdir build; cd build; cmake -DBOARD=acrn_efi_uos ..
      $ make

   Then we can find the zephyr.elf in samples/subsys/shell/shell_module/build/zephyr/.

   Build grub2 boot loader image

   .. code-block:: none

      $ cd $ZEPHYR_BASE
      $ ./boards/x86/common/scripts/build_grub.sh x86_64

   Find the binary at boards/x86/common/scripts/grub/bin/grub_x86_64.efi.

#. Preparing the boot device

   .. code-block:: none

      $ dd if=/dev/zero of=zephyr.img bs=1M count=1024
      $ mkfs.vfat -F 32 zephyr.img
      $ losetup -f -P --show zephyr.img
      $ mount /dev/loop0 /mnt

   Create the following directories.

   :file:`/mnt/efi`

   :file:`/mnt/efi/boot`

   :file:`/mnt/kernel/`

   Copy zephyr.elf to /mnt/kernel/ and grub_x86_64.efi to /mnt/efi/boot/.

   Create ``/mnt/efi/boot/grub.cfg`` containing the following:

   .. code-block:: console

     set default=0
     set timeout=10

     menuentry "Zephyr Kernel" {
         multiboot /kernel/zephyr.elf
     }

#. Follow the instructions found in the :ref:`getting-started-apl-nuc` to
   boot "The ACRN Service OS"

#. Boot Zephyr as User OS

   Copy the launch script ``devicemodel/samples/nuc/launch_zephyr.sh`` and ``zephyr.img`` 
   to ``SOS:$HOME/``.

   .. code-block:: none

      $ ./launch_zephyr.sh

   Load Zephyr using ``grub_x86_64.efi`` in UEFI shell:

   .. code-block:: console

      $ shell> FS0:
      $ FS0:\> efi\boot\grub_x86_64.efi

   Then acrn-dm will start zephyr as uos automatically.
