#!/usr/bin/env python3
# fix-encoding.py
# すべてのC++ファイルをUTF-8 BOMに変換

import os
import sys
from pathlib import Path

# UTF-8 BOMバイト
UTF8_BOM = b'\xef\xbb\xbf'

def has_utf8_bom(filepath):
    """ファイルがUTF-8 BOMを持っているか確認"""
    try:
        with open(filepath, 'rb') as f:
            header = f.read(3)
            return header == UTF8_BOM
    except:
        return False

def convert_to_utf8_bom(filepath):
    """ファイルをUTF-8 BOMに変換"""
    try:
        # 既にBOMがある場合はスキップ
        if has_utf8_bom(filepath):
            return 'skip'
        
        # ファイルを読み込み（エンコーディング自動検出）
        with open(filepath, 'r', encoding='utf-8-sig', errors='ignore') as f:
            content = f.read()
        
        # UTF-8 BOM付きで書き込み
        with open(filepath, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        
        # 検証
        if has_utf8_bom(filepath):
            return 'ok'
        else:
            return 'error'
    except Exception as e:
        print(f"    Error: {e}")
        return 'error'

def main():
    print("=" * 50)
    print("NeonVector - UTF-8 BOM Converter (Python)")
    print("=" * 50)
    print()
    
    # 検索する拡張子
    extensions = ['.cpp', '.h', '.hpp', '.c']
    
    # 検索するディレクトリ
    directories = ['src', 'include', 'examples', 'tests']
    
    total = 0
    converted = 0
    skipped = 0
    errors = 0
    
    for directory in directories:
        if not os.path.exists(directory):
            continue
        
        print(f"Processing: {directory}")
        
        for ext in extensions:
            for filepath in Path(directory).rglob(f'*{ext}'):
                total += 1
                relative_path = str(filepath)
                
                result = convert_to_utf8_bom(filepath)
                
                if result == 'ok':
                    print(f"  [OK]   {relative_path}")
                    converted += 1
                elif result == 'skip':
                    print(f"  [SKIP] {relative_path}")
                    skipped += 1
                else:
                    print(f"  [ERROR] {relative_path}")
                    errors += 1
        
        print()
    
    print("=" * 50)
    print("Summary:")
    print(f"  Total files:       {total}")
    print(f"  Converted:         {converted}")
    print(f"  Already UTF-8 BOM: {skipped}")
    print(f"  Errors:            {errors}")
    print("=" * 50)
    print()
    
    if converted > 0:
        print(f"✓ Successfully converted {converted} files to UTF-8 BOM")
        print()
        print("Next steps:")
        print("  1. Rebuild your project")
        print("  2. Verify no C4819 warnings")
    else:
        print("✓ All files are already UTF-8 BOM")

if __name__ == '__main__':
    main()