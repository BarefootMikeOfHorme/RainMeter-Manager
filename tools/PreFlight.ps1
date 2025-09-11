Param(
  [switch]$DryRun
)
Write-Host "RainmeterManager Pre-Flight Sync"
git --no-pager remote -v
git fetch --all --prune
$branch = (git rev-parse --abbrev-ref HEAD).Trim()
git --no-pager status
if (-not $DryRun) {
  git add -A
  git commit -m "chore(pre-flight): snapshot before v1.0 plan — sync backups and state [skip ci]" --allow-empty
  if (Test-Path "changes") { git add changes -A }
  if (Test-Path "dumps") { git add dumps -A }
  if (Test-Path "suggested widgets or ideas") { git add "suggested widgets or ideas" -A }
  git commit -m "chore(pre-flight): add tracked backups and reports [skip ci]" --allow-empty
  git push origin --all
  git push origin --tags
  if ((git remote) -contains "gitea") {
    git push gitea --all
    git push gitea --tags
  }
}