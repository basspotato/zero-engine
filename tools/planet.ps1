param([string]$in, [string]$out, [bool]$whiteBg, [double]$night)
# Crops a planet photo to its disc, fades the edge to transparent (inset so no
# backdrop pixels remain) and bakes in the night side lit from the upper left.
Add-Type -AssemblyName PresentationCore, WindowsBase
$fs = [IO.File]::OpenRead($in)
$dec = [Windows.Media.Imaging.BitmapDecoder]::Create($fs, 'PreservePixelFormat', 'OnLoad')
$f = New-Object Windows.Media.Imaging.FormatConvertedBitmap($dec.Frames[0], [Windows.Media.PixelFormats]::Bgra32, $null, 0)
$fs.Close()
$w=$f.PixelWidth; $h=$f.PixelHeight; $stride=$w*4; $px = New-Object byte[] ($stride*$h); $f.CopyPixels($px,$stride,0)

# Disc: rows/columns with enough planet pixels (ignores stray specks).
$cols = New-Object int[] $w; $rows = New-Object int[] $h
for($y=0;$y -lt $h;$y++){ for($x=0;$x -lt $w;$x++){ $i=$y*$stride+$x*4; $s=[int]$px[$i]+$px[$i+1]+$px[$i+2]
  $planet = if($whiteBg){$s -lt 690}else{$s -gt 70}
  if($planet){ $cols[$x]++; $rows[$y]++ } } }
$minX=($cols | ForEach-Object -Begin {$k=0} -Process { if($_ -gt 3){$k}; $k++ } | Select-Object -First 1)
$maxX=($cols | ForEach-Object -Begin {$k=0} -Process { if($_ -gt 3){$k}; $k++ } | Select-Object -Last 1)
$minY=($rows | ForEach-Object -Begin {$k=0} -Process { if($_ -gt 3){$k}; $k++ } | Select-Object -First 1)
$maxY=($rows | ForEach-Object -Begin {$k=0} -Process { if($_ -gt 3){$k}; $k++ } | Select-Object -Last 1)
$cx=($minX+$maxX)/2.0; $cy=($minY+$maxY)/2.0; $r=[Math]::Min($maxX-$minX,$maxY-$minY)/2.0
$size=[int][Math]::Ceiling(2*$r)+2
$outPx = New-Object byte[] ($size*$size*4)
$fade0 = 0.90; $fade1 = 0.965   # edge: opaque inside 0.90 r, gone by 0.965 r
$lx=-0.55; $ly=-0.55; $reach=2.05
for($y=0;$y -lt $size;$y++){ for($x=0;$x -lt $size;$x++){
  $u=($x+0.5-$size/2.0)/$r; $v=($y+0.5-$size/2.0)/$r; $d=[Math]::Sqrt($u*$u+$v*$v)
  if($d -ge $fade1){ continue }
  $sx=[int][Math]::Round($cx+$u*$r); $sy=[int][Math]::Round($cy+$v*$r)
  if($sx -lt 0 -or $sy -lt 0 -or $sx -ge $w -or $sy -ge $h){ continue }
  $i=$sy*$stride+$sx*4
  $t=[Math]::Min(1.0,[Math]::Max(0.0,($fade1-$d)/($fade1-$fade0))); $alpha=$t*$t*(3-2*$t)
  $s=[Math]::Sqrt(($u-$lx)*($u-$lx)+($v-$ly)*($v-$ly))/$reach
  if($s -lt 0.45){ $lift=0.12*(1-$s/0.45); $mul=1.0 } else { $lift=0; $mul=1.0-$night*[Math]::Min(1.0,($s-0.45)/0.55) }
  $o=($y*$size+$x)*4
  for($c=0;$c -lt 3;$c++){ $val=[double]$px[$i+$c]*$mul; $val=$val+(255-$val)*$lift; $outPx[$o+$c]=[byte][Math]::Min(255,[Math]::Round($val)) }
  $outPx[$o+3]=[byte][Math]::Round(255*$alpha)
} }
$bmp=[Windows.Media.Imaging.BitmapSource]::Create($size,$size,96,96,[Windows.Media.PixelFormats]::Bgra32,$null,$outPx,$size*4)
$enc=New-Object Windows.Media.Imaging.PngBitmapEncoder; $enc.Frames.Add([Windows.Media.Imaging.BitmapFrame]::Create($bmp))
$o=[IO.File]::Create($out); $enc.Save($o); $o.Close()
"$out disc $minX,$minY-$maxX,$maxY r=$r size=$size"
