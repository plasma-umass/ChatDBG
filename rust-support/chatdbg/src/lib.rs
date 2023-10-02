pub use chatdbg_macros::main;

use std::fs::{File, OpenOptions};
use std::io::Write;
use std::panic;
use std::sync::{
    atomic::{AtomicBool, Ordering},
    Mutex,
};
use std::thread;

// Global Mutex to synchronize file writes across threads.
lazy_static::lazy_static! {
    static ref FILE_MUTEX: Mutex<()> = Mutex::new(());
    static ref FILE_CREATED: AtomicBool = AtomicBool::new(false);
}

pub fn chatdbg() {
    // Set a custom panic hook.
    panic::set_hook(Box::new(|info| {
        let _guard = FILE_MUTEX.lock().unwrap(); // Lock Mutex to synchronize access.

        // Format the panic message similarly to the default panic handler.
        let payload = if let Some(s) = info.payload().downcast_ref::<&str>() {
            *s
        } else {
            "Box<Any>"
        };

        let location = if let Some(location) = info.location() {
            format!(" at '{}' line {}", location.file(), location.line())
        } else {
            String::from("")
        };

        let message = format!(
            "thread '{}' panicked with '{}'{}",
            thread::current().name().unwrap_or("<unnamed>"),
            payload,
            location
        );

        // Print to stderr.
        eprintln!("{}", message);

        // Specify the filename without including the process id.
        let filename = "panic_log.txt";

        // Open the file with appropriate options.
        let mut file = if FILE_CREATED.swap(true, Ordering::SeqCst) {
            // If the file is already created by another thread, open it in append mode.
            OpenOptions::new()
                .create(true)
                .append(true)
                .open(filename)
                .expect("Unable to open file")
        } else {
            // If this is the first thread to create the file, overwrite any existing file.
            File::create(filename).expect("Unable to create file")
        };

        // Write to the file.
        writeln!(file, "{}", message).expect("Unable to write to file");
    }));
}
