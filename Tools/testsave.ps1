# 테스트용 세이브 편집기 — 게임 코드는 건드리지 않는다.
#
# 상점을 시험하려면 재화가 필요한데, 재화는 게임을 해야만 벌린다. 빌드에 치트 키를
# 넣으면 그게 제출본에도 들어가므로, 대신 save.dat를 직접 쓴다. 배포물은 그대로다.
#
#   .\tools\testsave.ps1 -Coins 999
#   .\tools\testsave.ps1 -Coins 999 -AllSkins
#   .\tools\testsave.ps1 -Show
#   .\tools\testsave.ps1 -Clear          # 구매 내역만 비움 (다시 살 수 있게)
#
# ⚠ 게임을 끄고 실행할 것. 게임은 설정 변경·구매·신기록에서 save.dat를 덮어쓰므로,
#   켜 둔 채로 바꾸면 다음 저장에 날아간다.

param(
    [int]    $Coins    = -1,      # -1 = 건드리지 않음
    [switch] $AllSkins,
    [switch] $Clear,              # pendCard / pendHp / pendBurst / 리롤 횟수 초기화
    [switch] $Show,
    [string] $Path    = ""        # save.dat 경로. 비우면 아래 순서로 찾는다
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# V4 레이아웃. main.c의 SaveData와 순서가 같아야 한다.
$MAGIC = 0x4C425434     # 'LBT4'
$SIZE  = 76
$OFF = @{
    magic = 0;  hi0 = 4;   hi1 = 8;   volStep = 12; reducedMotion = 16
    borderless = 20; mode = 24; lang = 28; wallet = 32
    shopOffer0 = 36; shopOffer1 = 40; shopOffer2 = 44
    shopRerolls = 48; pendCard = 52; pendHp = 56; pendBurst = 60
    skinOwned = 64; skinSel = 68; pendCard2 = 72
}

if (-not $Path) {
    foreach ($c in @("$root\dist\save.dat", "$root\LowByteGame\x64\Release\save.dat")) {
        if (Test-Path $c) { $Path = $c; break }
    }
    if (-not $Path) { $Path = "$root\dist\save.dat" }   # 없으면 여기에 새로 만든다
}

function Get-I32($b, $o) { [BitConverter]::ToInt32($b, $o) }
function Set-I32($b, $o, $v) {
    [Array]::Copy([BitConverter]::GetBytes([int32]$v), 0, $b, $o, 4)
}

# 기존 파일을 읽되, 없거나 형식이 다르면 기본값으로 새로 만든다.
if ((Test-Path $Path) -and ((Get-Item $Path).Length -eq $SIZE) -and
    ([BitConverter]::ToUInt32([System.IO.File]::ReadAllBytes($Path), 0) -eq $MAGIC)) {
    $b = [System.IO.File]::ReadAllBytes($Path)
    $fresh = $false
}
else {
    $b = New-Object byte[] $SIZE
    Set-I32 $b $OFF.magic       $MAGIC
    Set-I32 $b $OFF.volStep     8
    Set-I32 $b $OFF.lang        1        # 한국어
    Set-I32 $b $OFF.skinOwned   1        # 0번 스킨만
    Set-I32 $b $OFF.pendCard   (-1)
    Set-I32 $b $OFF.pendCard2  (-1)
    Set-I32 $b $OFF.shopOffer0 (-1)
    Set-I32 $b $OFF.shopOffer1 (-1)
    Set-I32 $b $OFF.shopOffer2 (-1)
    $fresh = $true
}

if (-not $Show) {
    if ($Coins -ge 0) { Set-I32 $b $OFF.wallet $Coins }
    if ($AllSkins)    { Set-I32 $b $OFF.skinOwned 0xFF }   # 스킨 8종 전부
    if ($Clear) {
        Set-I32 $b $OFF.pendCard   (-1)
        Set-I32 $b $OFF.pendCard2  (-1)
        Set-I32 $b $OFF.pendHp      0
        Set-I32 $b $OFF.pendBurst   0
        Set-I32 $b $OFF.shopRerolls 0
    }
    $dir = Split-Path -Parent $Path
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    [System.IO.File]::WriteAllBytes($Path, $b)
}

$owned = Get-I32 $b $OFF.skinOwned
$n = 0; for ($i = 0; $i -lt 8; $i++) { if ($owned -band (1 -shl $i)) { $n++ } }

"{0}  ({1})" -f $Path, $(if ($fresh) { "새로 만듦" } else { "기존 파일" })
"  재화          {0}"      -f (Get-I32 $b $OFF.wallet)
"  최고점수      노말 {0} / 하드 {1}" -f (Get-I32 $b $OFF.hi0), (Get-I32 $b $OFF.hi1)
"  스킨          {0}/8 해금 (선택 {1})" -f $n, (Get-I32 $b $OFF.skinSel)
"  다음 판 카드  {0}, {1}" -f (Get-I32 $b $OFF.pendCard), (Get-I32 $b $OFF.pendCard2)
"  체력/버스트   {0} / {1}" -f (Get-I32 $b $OFF.pendHp), (Get-I32 $b $OFF.pendBurst)
"  리롤 횟수     {0}"      -f (Get-I32 $b $OFF.shopRerolls)
