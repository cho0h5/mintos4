use crate::ffi::kMemCpy;

#[repr(C, packed)]
struct Queue {
    data_size: u32,
    max_data_count: u32,

    queue_array: *mut u8,
    put_index: u32,
    get_index: u32,

    last_operation_put: bool,
}

impl Queue {
    #[no_mangle]
    pub extern "C" fn kInitializeQueue(queue: *mut Queue, queue_buffer: *mut u8, max_data_count: u32, data_size: u32) {
        let mut queue = unsafe { &mut *queue };

        queue.max_data_count = max_data_count;
        queue.data_size = data_size;
        queue.queue_array = queue_buffer;
        queue.put_index = 0;
        queue.get_index = 0;
        queue.last_operation_put = false;
    }

    #[no_mangle]
    pub extern "C" fn kIsQueueFull(&self) -> bool {
        return self.get_index == self.put_index && self.last_operation_put;
    }

    #[no_mangle]
    pub extern "C" fn kIsQueueEmpty(&self) -> bool {
        return self.get_index == self.put_index && !self.last_operation_put;
    }

    #[no_mangle]
    pub extern "C" fn kPutQueue(&mut self, data: *const u8) -> bool {
        if self.kIsQueueFull() {
            return false;
        }

        unsafe {
            kMemCpy(
                self.queue_array.add((self.data_size * self.put_index) as usize),
                data,
                self.data_size,
            );
        }
        let next = self.put_index + 1;
        self.put_index = if next >= self.max_data_count { 0 } else { next };
        self.last_operation_put = true;
        return true;
    }

    #[no_mangle]
    pub extern "C" fn kGetQueue(&mut self, data: *mut u8) -> bool {
        if self.kIsQueueEmpty() {
            return false;
        }

        unsafe {
            kMemCpy(
                data,
                self.queue_array.add((self.data_size * self.get_index) as usize),
                self.data_size,
            );
        }
        let next = self.get_index + 1;
        self.get_index = if next >= self.max_data_count { 0 } else { next };
        self.last_operation_put = false;
        return true;
    }
}