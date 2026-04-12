#![no_std]
#![no_main]

mod queue;

pub mod ffi {
    extern "C" {
        pub fn kMemCpy(dst: *mut u8, src: *const u8, count: u32) -> u32;
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
