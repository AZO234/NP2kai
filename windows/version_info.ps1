Param (
 [String]$Namespace,
 [String]$Project,
 [String]$GitRoot,
 [String]$HeaderFile="np2_git_version.h",
 [String]$VerPrefix="https://github.com/AZO234/NP2kai/commit/"
)

Push-Location -LiteralPath $GitRoot

$VerFileHead = "`#ifndef _NP2_VERGIT_H_`n`#define _NP2_VERGIT_H_`n`n"
$VerFileTail = "`n`#endif  // _NP2_VERGIT_H_"

$VerBy    = (git log -n 1 --format=format:"`#define VER_AUTHER `\`"%an `<%ae`>`\`"%n") | Out-String
$VerUrl   = (git log -n 1 --format=format:"`#define VER_URL `\`"$VerPrefix%H`\`"%n") | Out-String
$VerDate  = (git log -n 1 --format=format:"`#define VER_DATE `\`"%ai`\`"%n") | Out-String
$VerSubj  = (git log -n 1 --format=format:"`#define VER_SUBJECT `\`"%f`\`"%n") | Out-String
$VerHash  = ("`#define VER_HASH `"" + (git rev-parse HEAD) + "`"") | Out-String
$VerSHash = ("`#define VER_SHASH `"" + (git rev-parse --short HEAD) + "`"") | Out-String

$VerChgs = ((git ls-files --exclude-standard -d -m -o -k) | Measure-Object -Line).Lines

if ($VerChgs -gt 0) {
  $VerDirty = "`#define VER_DIRTY TRUE`n"
  $VerDStr = "#define VER_DSTR `"Dirty`"`n" | Out-String
} else {
  $VerDirty = "`#define VER_DIRTY FALSE`n"
  $VerDStr = "`#define VER_DSTR `"`"`n" | Out-String
}

"Written $Project\" + (
  New-Item -Force -Path "$Project" -Name "$HeaderFile" -ItemType "file" -Value "$VerFileHead$VerUrl$VerDate$VerSubj$VerBy$VerHash$VerSHash$VerDirty$VerDStr$VerFileTail"
).Name + " as:"
""
Get-Content "$Project\$HeaderFile"
""
