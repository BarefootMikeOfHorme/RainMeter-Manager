#!/usr/bin/env python3
"""
AXIOM Utility Scripts
Helper tools for managing AXIOM scraper
"""

import argparse
import sys
from pathlib import Path
import json
import sqlite3
from datetime import datetime
import shutil


class AXIOMUtilities:
    """Utility commands for AXIOM scraper"""
    
    def __init__(self, db_path='scraped_data/skins.db'):
        self.db_path = Path(db_path)
        if not self.db_path.exists():
            print(f"Error: Database not found at {db_path}")
            print("Run the scraper first to create the database")
            sys.exit(1)
    
    def get_connection(self):
        """Get database connection"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        return conn
    
    def stats(self):
        """Show detailed statistics"""
        print("\n" + "=" * 70)
        print("AXIOM COLLECTION STATISTICS")
        print("=" * 70 + "\n")
        
        conn = self.get_connection()
        
        # Total skins
        cursor = conn.execute("SELECT COUNT(*) FROM skins")
        total = cursor.fetchone()[0]
        print(f"📊 Total Skins: {total}")
        print()
        
        # By status
        print("Status Breakdown:")
        cursor = conn.execute("""
            SELECT download_status, COUNT(*) as count 
            FROM skins 
            GROUP BY download_status 
            ORDER BY count DESC
        """)
        for row in cursor:
            percentage = (row['count'] / total * 100) if total > 0 else 0
            print(f"  {row['download_status']:20} {row['count']:6} ({percentage:5.1f}%)")
        print()
        
        # By category
        print("Top 10 Categories:")
        cursor = conn.execute("""
            SELECT category, COUNT(*) as count 
            FROM skins 
            GROUP BY category 
            ORDER BY count DESC 
            LIMIT 10
        """)
        for row in cursor:
            print(f"  {row['category']:30} {row['count']:6}")
        print()
        
        # Top authors
        print("Top 10 Authors:")
        cursor = conn.execute("""
            SELECT author, COUNT(*) as count 
            FROM skins 
            WHERE author != 'Unknown Author'
            GROUP BY author 
            ORDER BY count DESC 
            LIMIT 10
        """)
        for row in cursor:
            print(f"  {row['author']:30} {row['count']:6}")
        print()
        
        # Recent additions
        print("Most Recently Scraped (Last 5):")
        cursor = conn.execute("""
            SELECT title, category, scraped_at 
            FROM skins 
            ORDER BY scraped_at DESC 
            LIMIT 5
        """)
        for row in cursor:
            print(f"  {row['title'][:40]:40} [{row['category']}]")
        print()
        
        conn.close()
    
    def reset(self, category=None, status=None):
        """Reset download status"""
        conn = self.get_connection()
        
        if category:
            print(f"Resetting skins in category: {category}")
            cursor = conn.execute("""
                UPDATE skins 
                SET download_status = 'pending', 
                    local_path = '', 
                    extracted_path = '', 
                    file_hash = ''
                WHERE category = ?
            """, (category,))
        elif status:
            print(f"Resetting skins with status: {status}")
            cursor = conn.execute("""
                UPDATE skins 
                SET download_status = 'pending', 
                    local_path = '', 
                    extracted_path = '', 
                    file_hash = ''
                WHERE download_status = ?
            """, (status,))
        else:
            print("Error: Specify --category or --status")
            conn.close()
            return
        
        count = cursor.rowcount
        conn.commit()
        conn.close()
        
        print(f"✅ Reset {count} skins to pending status")
        print("Run scraper with --resume to re-download")
    
    def export(self, format='json', output=None):
        """Export data in various formats"""
        conn = self.get_connection()
        
        if output is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output = f"axiom_export_{timestamp}.{format}"
        
        print(f"Exporting to {output}...")
        
        if format == 'json':
            cursor = conn.execute("SELECT * FROM skins")
            skins = [dict(row) for row in cursor.fetchall()]
            
            with open(output, 'w', encoding='utf-8') as f:
                json.dump(skins, f, indent=2, ensure_ascii=False)
        
        elif format == 'csv':
            import csv
            cursor = conn.execute("SELECT * FROM skins")
            rows = cursor.fetchall()
            
            if rows:
                with open(output, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                    writer.writeheader()
                    for row in rows:
                        writer.writerow(dict(row))
        
        elif format == 'txt':
            cursor = conn.execute("SELECT * FROM skins")
            with open(output, 'w', encoding='utf-8') as f:
                for row in cursor:
                    f.write(f"Title: {row['title']}\n")
                    f.write(f"Author: {row['author']}\n")
                    f.write(f"Category: {row['category']}\n")
                    f.write(f"Status: {row['download_status']}\n")
                    f.write(f"URL: {row['url']}\n")
                    f.write("-" * 70 + "\n\n")
        
        else:
            print(f"Unsupported format: {format}")
            conn.close()
            return
        
        conn.close()
        print(f"✅ Exported to {output}")
    
    def search(self, query, field='title'):
        """Search for skins"""
        conn = self.get_connection()
        
        print(f"\nSearching {field} for: {query}")
        print("=" * 70)
        
        cursor = conn.execute(f"""
            SELECT title, author, category, download_status, url 
            FROM skins 
            WHERE {field} LIKE ? 
            ORDER BY title
        """, (f"%{query}%",))
        
        results = cursor.fetchall()
        
        if results:
            print(f"\nFound {len(results)} results:\n")
            for row in results:
                status_icon = {
                    'pending': '⏳',
                    'downloaded': '📥',
                    'extracted': '✅',
                    'download_failed': '❌',
                    'extraction_failed': '⚠️'
                }.get(row['download_status'], '❓')
                
                print(f"{status_icon} {row['title']}")
                print(f"   Author: {row['author']}")
                print(f"   Category: {row['category']}")
                print(f"   URL: {row['url']}")
                print()
        else:
            print("No results found")
        
        conn.close()
    
    def cleanup(self):
        """Clean up orphaned files"""
        print("🧹 Cleaning up orphaned files...")
        
        conn = self.get_connection()
        cursor = conn.execute("SELECT local_path FROM skins WHERE local_path != ''")
        db_files = {row['local_path'] for row in cursor.fetchall()}
        conn.close()
        
        # Check downloads directory
        downloads_dir = self.db_path.parent / "downloads"
        if not downloads_dir.exists():
            print("Downloads directory not found")
            return
        
        orphaned = []
        for file_path in downloads_dir.rglob("*"):
            if file_path.is_file():
                if str(file_path) not in db_files:
                    orphaned.append(file_path)
        
        if orphaned:
            print(f"\nFound {len(orphaned)} orphaned files:")
            for path in orphaned[:10]:  # Show first 10
                print(f"  - {path.name}")
            if len(orphaned) > 10:
                print(f"  ... and {len(orphaned) - 10} more")
            
            response = input("\nDelete orphaned files? (y/n): ")
            if response.lower() == 'y':
                for path in orphaned:
                    path.unlink()
                print(f"✅ Deleted {len(orphaned)} orphaned files")
            else:
                print("Cleanup cancelled")
        else:
            print("✅ No orphaned files found")
    
    def backup(self):
        """Create backup of database"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_dir = self.db_path.parent / "backups"
        backup_dir.mkdir(exist_ok=True)
        
        backup_path = backup_dir / f"skins_backup_{timestamp}.db"
        
        print(f"Creating backup: {backup_path}")
        shutil.copy2(self.db_path, backup_path)
        print(f"✅ Backup created successfully")
        print(f"   Size: {backup_path.stat().st_size / (1024*1024):.2f} MB")
    
    def integrity_check(self):
        """Check database integrity"""
        print("🔍 Checking database integrity...")
        
        conn = self.get_connection()
        
        # SQLite integrity check
        cursor = conn.execute("PRAGMA integrity_check")
        result = cursor.fetchone()[0]
        
        if result == 'ok':
            print("✅ Database integrity: OK")
        else:
            print(f"❌ Database integrity issues: {result}")
        
        # Check for nulls in required fields
        cursor = conn.execute("""
            SELECT COUNT(*) FROM skins 
            WHERE url IS NULL OR title IS NULL OR category IS NULL
        """)
        null_count = cursor.fetchone()[0]
        
        if null_count > 0:
            print(f"⚠️  Found {null_count} records with NULL required fields")
        else:
            print("✅ All required fields populated")
        
        # Check for duplicate URLs
        cursor = conn.execute("""
            SELECT url, COUNT(*) as count 
            FROM skins 
            GROUP BY url 
            HAVING count > 1
        """)
        duplicates = cursor.fetchall()
        
        if duplicates:
            print(f"⚠️  Found {len(duplicates)} duplicate URLs")
        else:
            print("✅ No duplicate URLs")
        
        conn.close()
    
    def vacuum(self):
        """Compact database"""
        print("🗜️  Compacting database...")
        
        before_size = self.db_path.stat().st_size
        
        conn = self.get_connection()
        conn.execute("VACUUM")
        conn.close()
        
        after_size = self.db_path.stat().st_size
        saved = before_size - after_size
        
        print(f"✅ Database compacted")
        print(f"   Before: {before_size / (1024*1024):.2f} MB")
        print(f"   After:  {after_size / (1024*1024):.2f} MB")
        print(f"   Saved:  {saved / (1024*1024):.2f} MB")


def main():
    """Main CLI interface"""
    parser = argparse.ArgumentParser(
        description="AXIOM Utility Tools",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  axiom_utils.py stats                    Show statistics
  axiom_utils.py export --format json     Export to JSON
  axiom_utils.py search "weather"         Search for weather skins
  axiom_utils.py reset --status failed    Reset failed downloads
  axiom_utils.py backup                   Create database backup
  axiom_utils.py cleanup                  Clean orphaned files
        """
    )
    
    parser.add_argument('--db', default='scraped_data/skins.db', help='Database path')
    
    subparsers = parser.add_subparsers(dest='command', help='Command to run')
    
    # Stats command
    subparsers.add_parser('stats', help='Show collection statistics')
    
    # Export command
    export_parser = subparsers.add_parser('export', help='Export data')
    export_parser.add_argument('--format', choices=['json', 'csv', 'txt'], default='json')
    export_parser.add_argument('--output', help='Output filename')
    
    # Search command
    search_parser = subparsers.add_parser('search', help='Search for skins')
    search_parser.add_argument('query', help='Search query')
    search_parser.add_argument('--field', default='title', 
                              choices=['title', 'author', 'category', 'description'])
    
    # Reset command
    reset_parser = subparsers.add_parser('reset', help='Reset download status')
    reset_parser.add_argument('--category', help='Reset specific category')
    reset_parser.add_argument('--status', help='Reset specific status')
    
    # Cleanup command
    subparsers.add_parser('cleanup', help='Clean orphaned files')
    
    # Backup command
    subparsers.add_parser('backup', help='Create database backup')
    
    # Integrity check
    subparsers.add_parser('check', help='Check database integrity')
    
    # Vacuum command
    subparsers.add_parser('vacuum', help='Compact database')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    utils = AXIOMUtilities(args.db)
    
    if args.command == 'stats':
        utils.stats()
    elif args.command == 'export':
        utils.export(format=args.format, output=args.output)
    elif args.command == 'search':
        utils.search(args.query, args.field)
    elif args.command == 'reset':
        utils.reset(category=args.category, status=args.status)
    elif args.command == 'cleanup':
        utils.cleanup()
    elif args.command == 'backup':
        utils.backup()
    elif args.command == 'check':
        utils.integrity_check()
    elif args.command == 'vacuum':
        utils.vacuum()


if __name__ == "__main__":
    main()
