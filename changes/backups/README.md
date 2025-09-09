# Backups and change logs

This directory contains backups and related change logs that are tracked in Git from now on.

Legacy backups (pre-Git)
- The files in the legacy/ subfolder were produced before Git tracking was established.
- They are not current and should be treated as historical snapshots only.
- Date of legacy archives currently present: 2025-08-08.

Guidance
- Do not restore legacy backups directly to overwrite current work.
- If you need to inspect them, extract into a temporary directory and compare using diffs.
- New backups should be committed under this directory with clear timestamps, e.g. backups/YYYYMMDD_HHMMSS/.

