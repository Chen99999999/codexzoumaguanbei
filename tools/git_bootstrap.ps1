param(
    [string]$RepoRoot = "C:\Users\Administrator\Desktop\智能车\zoumaguanbei_cursor_work",
    [string]$RemoteUrl = "",
    [string]$Branch = "main",
    [string]$UserName = "",
    [string]$UserEmail = ""
)

function Resolve-GitExe {
    $candidates = @(
        "git",
        "C:\Program Files\Git\cmd\git.exe",
        "C:\Program Files\Git\bin\git.exe",
        "C:\Program Files (x86)\Git\cmd\git.exe",
        "C:\Program Files (x86)\Git\bin\git.exe"
    )

    foreach ($candidate in $candidates) {
        try {
            if ($candidate -eq "git") {
                $cmd = Get-Command git -ErrorAction Stop
                return $cmd.Source
            }

            if (Test-Path $candidate) {
                return $candidate
            }
        } catch {
        }
    }

    throw "没找到 git。先安装 Git for Windows，或把 git.exe 路径加到 PATH。"
}

function Invoke-Git {
    param(
        [string]$GitExe,
        [string[]]$Args
    )

    & $GitExe @Args
    if ($LASTEXITCODE -ne 0) {
        throw "git 命令失败: $($Args -join ' ')"
    }
}

$gitExe = Resolve-GitExe
Write-Host "[git] using $gitExe"

if (-not (Test-Path $RepoRoot)) {
    throw "RepoRoot 不存在: $RepoRoot"
}

Push-Location $RepoRoot
try {
    if (-not (Test-Path ".git")) {
        Invoke-Git -GitExe $gitExe -Args @("init", "-b", $Branch)
    } else {
        Write-Host "[git] .git already exists"
    }

    if ($UserName) {
        Invoke-Git -GitExe $gitExe -Args @("config", "user.name", $UserName)
    }

    if ($UserEmail) {
        Invoke-Git -GitExe $gitExe -Args @("config", "user.email", $UserEmail)
    }

    Invoke-Git -GitExe $gitExe -Args @("add", ".")

    & $gitExe diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        Invoke-Git -GitExe $gitExe -Args @("commit", "-m", "feat: bridge v3 chassis with award vision layer")
    } else {
        Write-Host "[git] no staged changes to commit"
    }

    if ($RemoteUrl) {
        & $gitExe remote get-url origin 1>$null 2>$null
        if ($LASTEXITCODE -eq 0) {
            Invoke-Git -GitExe $gitExe -Args @("remote", "set-url", "origin", $RemoteUrl)
        } else {
            Invoke-Git -GitExe $gitExe -Args @("remote", "add", "origin", $RemoteUrl)
        }

        Invoke-Git -GitExe $gitExe -Args @("branch", "-M", $Branch)
        Invoke-Git -GitExe $gitExe -Args @("push", "-u", "origin", $Branch)
    } else {
        Write-Host "[git] remote not set; local repo initialized and committed only"
    }
}
finally {
    Pop-Location
}
