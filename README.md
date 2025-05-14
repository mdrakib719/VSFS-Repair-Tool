# 🛠️ VSFS Repair Tool

**CSE321 Lab Project 02 – Group 08**  
Authors: Md. Rakib Hossain, Dewan M.I. Mukto, Mahmuda Akter Nipa

---

## 📌 Project Overview

The **VSFS Repair Tool** is a low-level filesystem checker and repair utility for a virtual file system (VSFS – Virtual Simple File System). It is designed to analyze and fix inconsistencies in the VSFS image, such as:

- Invalid inodes
- Misconfigured inode and data bitmaps
- Incorrect data block assignments
- Duplicate or orphaned blocks

This tool simulates how real-world filesystem tools like `fsck` work and demonstrates key Operating System concepts such as disk structure, block mapping, and file metadata management.

---

## 🔧 Features

- Validate the superblock (magic number, block size, total block count)
- Scan inodes and fix:
  - Incorrect inode bitmap entries
  - Invalid or duplicate data block pointers
- Repair the data bitmap to reflect correct block usage
- Logs every correction with informative messages
- Uses ANSI color codes for clean terminal output

---

## 🧪 How It Works

- Reads and validates the Superblock
- Parses the inode bitmap and data bitmap
- Traverses the inode table:
  - Checks if inodes are valid (non-zero links and not deleted)
  - Ensures inodes match their bitmap status
  - Detects and removes invalid or duplicate direct pointers
- Updates the data bitmap to ensure it only marks used blocks
- Writes back any changes to the file

---

## 📁 VSFS Structure

- **Block Size**: 4096 bytes  
- **Total Blocks**: 64  
- **Superblock**: Block 0  
- **Inode Bitmap**: Block 1  
- **Data Bitmap**: Block 2  
- **Inode Table**: Blocks 3–7  
- **Data Blocks**: Blocks 8–63  

---

## 🚀 Usage

1. **Compile the program:**

   ```bash
   gcc -o vsfs_repair_tool main.c


   ```for run
   ./vsfs_repair_tool
