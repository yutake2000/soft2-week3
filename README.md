# 構成
- paint4.c
	- メインのプログラム
- paint4.h
	- 関数・構造体の定義を行うヘッダファイル
	- 大まかな説明が書かれている
- layer.c
	- レイヤー操作の関数が多いためpaint4.cから分割した
	- paint4.cでincludeしている

- sample(フォルダ)
	- load, animateコマンドで読み込めるファイルが入っている

# 実行方法

## コンパイル
```
gcc paint4.c -o paint4 -lm
```

## 実行
```
./paint4 <width> <height> [pallet]

例:
./paint4 40 40
./paint4 100 40 pallet
```
- サンプルは基本40×40マスを想定している
- palletオプション
	- これを入れるとキャンバスの右側にパレットが表示される
	- colorコマンドでは色を数字で指定するので、このパレットに書かれて数字を使う

## サンプル
```
./paint4 40 40
```
- 上のコマンドをターミナルで実行した後、下記のファイルを読み込むコマンドをプログラム中で入力する
- animateコマンド実行後はEnterキーを押すごとにコマンドが1つずつ実行されていく(実際にはredoしている)
- 1つのファイルが終了したら次のanimateコマンドを実行する

```
animate sample/merge.txt
animate sample/chuju.txt
animate sample/emblem.txt
animate sample/osaka.txt
animate sample/osaka_cursor.txt
animate sample/nagasaki.txt
animate sample/google_photo_backet.txt
animate sample/google_photo_clip.txt
animate sample/google_play.txt
animate sample/google_play2.txt
animate sample/yamaguchi.txt
```
- sample/merge.txt
	- 楽書き
	- 普通のペンとマーカーの重なり方やレイヤーの重なり方がわかるサンプル

- sample/chuju.txt
	- クリッピングのサンプル

- sample/emblem.txt
	- クリッピングのサンプル

- osaka.txt
	- 大阪大学のロゴ
	- 円描画とレイヤーの移動・反転機能を主に使った

- osaka_cursor.txt
	- カーソルで3点を指定して、そこを通る弧の描画のみで作った
	- カーソルはコマンドで動かしている

- nagasaki.txt
	- 長崎大学の略式ロゴ
	- 使った機能はosaka.txtとほぼ同じ

- google_photo_backet.txt
	- レイヤーの反転とバケツ機能を主に使っている

- sample/google_photo_clip.txt
	- バケツ機能の代わりにクリッピングを利用して色をつけた

- sample/google_play.txt
	- 多角形描画を使った

- sample/google_play2.txt
	- マーカーを使ったバージョン

- sample/yamaguchi.txt
	- 山口県の県章
	- カーソル操作を用いた
	- 目印をつけておくことで描画コマンドで引数を省略できる

# コマンド一覧

## コマンドの見方
|括弧|意味|コマンド表記例|コマンド入力例|
|-|-|-|-|
|なし|書いてある文字列をそのまま入力する。|`undo`|`undo`|
|山括弧<br><>|適切な値(整数または文字列)を入れる。|`chpen <pen>`|`chpen #`<br>`chpen *`|
|角括弧<br>[]|オプション。必須ではない。|`backet <x> <y> [strict]`|`backet 3 14`<br>`backet 15 9 strict`|
|丸括弧<br>()|縦棒(\|)で区切られた文字列の<br>どれか1つを指定する。|`reverse (h \| v \| d)`|`reverse h`<br>`reverse d`|

## 図形
```
line <x0> <y0> <x1> <y1>
例: line 5 10 15 20
```
- (x0, y0)と(x1, y1)を端点とする線分を描画する
- 例では(5, 10)と(15, 20)を端点とする線分を描画する。

```
rect <x0> <y0> <width> <height>
fillrect <x0> <y0> <width> <height>
```
- (x0, y0)を左上とする幅width,高さheightの長方形を描画する
- fillが付いている方は中を塗りつぶした図形になる(以降も同様)

```
circle <x0> <y0> <r>
fillcircle <x0> <y0> <r>
```
- (x0, y0)を中心とする半径rの円を描画する

```
sector <x0> <y0> <r> <θ1> <θ2>
fillsector <x0> <y0> <r> <θ1> <θ2>
例:
sector 20 20 10 -120 -60
sector 20 20 10 240 300
fillsector 0 0 40 -60 -30
```
- (x0, y0)を中心とする半径r, 角度θ1からθ2までの弧を描画する
- θ1・θ2はx軸方向を0度とした反時計周りの角度(-90度以上)
- arcとsectorのような気もするが、他のコマンドと合わせた

```
polygon <x0> <y0> <x1> <y1> ...
fillpolygon <x0> <y0> <x1> <y1> ...
```
- (x0, y0), (x1, y1), ... を頂点とする多角形を描画する

## ツール
```
chpen <pen>
```
- ペンをpen(半角1文字)に変更する
- デフォルトは「\*」

```
color <color code>
```
- 色を変更する(0 - 255)
- プログラム実行時に`./paint4 40 40 pallet`のようにpalletオプションをつけるとカラーコードを見ることができる


```
marker
```
- ペンをマーカーに変更する
- 普通のペン(文字)で描いた部分は上書きするが、逆にこの上にペンを重ねて描くことはできる(sample/merge.txt参照)
- 戻す場合はchpenコマンドを用いる

```
eraser
```
- ペンを消しゴムに変更する
- 戻す場合はchpenコマンドを用いる


```
pensize <size>
```
- ペンの太さをsize(整数)に変更する
- デフォルトは1
- 線や多角形の場合は(2 * size + 1)ピクセル、円の場合はsizeピクセル程度の太さになる

```
backet <x> <y> [strict]
```
- (x, y)と連結している部分を塗りつぶす
- 基本は周り8マスに同じ文字・色・背景色(マーカー)があれば連結とみなす
- strictオプションをつけた場合は上下左右の4マスが対象

## コマンド
```
undo
```
- コマンドを一つ取り消す

```
redo
```
- コマンドを一つやり直す
- 何も入力せずエンターキーを押してもredoになる

```
save [<filename>]
```
- コマンド履歴をテキスト形式で保存する
- filenameを指定しない場合はhistory.txtに保存される

```
load [<filename>]
```
- コマンド履歴を読み込み全て実行する
- filenameを指定しない場合はhistory.txtを読み込む
- それまでの履歴は削除される

```
animate [<filename>]
```
- コマンド履歴を読み込むが実行はしない(全てundoしたような状態)
- redoで一つずつ実行していくことができる(上述の通りEnterキーを押すだけでよい)

```
quit
```
- プログラムを終了する

## レイヤー
```
layer add
```
- レイヤーを追加し、そのレイヤーに変更する

```
layer rm [<index>]
```
- レイヤーを削除する
- indexを省略した場合は現在のレイヤーを対象とする

```
layer ch <index>
```
- レイヤーを変更する

```
layer insert <index>
```
- 新しいレイヤーを指定した位置に挿入する

```
layer mv <target index> <dest index>
```
- レイヤーを並び替える(targetをdestに移動)

```
layer show [<index>]
layer hide [<index>]
```
- レイヤーの表示/非表示を切り替える
- indexを省略した場合は現在のレイヤーを対象とする
- 0を指定した場合は現在のレイヤー以外全てを対象とする

```
layer cp [<index>]
```
- レイヤーを複製し、そのレイヤーに変更する

```
layer merge [<index>]
```
- 下のレイヤーと結合する

```
layer (clip|unclip) [<index>]
```
- レイヤーをクリッピングする
- クリッピングすると、下のレイヤーで何も書かれていない部分は表示されなくなる
- 1つのレイヤーに複数のレイヤーをクリッピングすることも可能
- レイヤー1-3のうちレイヤー2,3がクリッピングされている場合、レイヤー1に何も書かれていない部分はレイヤー2,3で表示されなくなる
- sample/chuju.txt参照

## キャンバス操作

```
resize <width> <height>
```
- 盤面のサイズを変更する

```
aspect <ratio>
```
- 幅をratio倍に拡大する
- デフォルトは1
- 2にすると幅と高さが大体同じになり、円が真円のように見える

```
copy <x0> <y0> <width> <height>
```
- 指定した矩形部分をコピーしてクリップボードに保存する

```
paste <x0> <y0>
```
- 指定した部分にペーストする
- クリップボードに保存してあるのでレイヤーを跨いでも大丈夫

```
mv <x> <y> <width> <height> <dx> <dy>
```
- 選択した矩形部分を右にdx,下にdyだけ動かす

```
trim <x> <y> <width> <height>
```
- トリミングする

```
reverse (horizontal | vertical | diagonal)
reverse (h | v | d)
```
- 左右・上下・対角線で反転させる

## カーソル操作
```
cset <x> <y>
```
- (x, y)にカーソルを置く

```
cmv <dx> <dy>
```
- 右にdx, 下にdyだけカーソルを移動させる

```
cmark
```
- 現在のカーソル位置に目印をつける
- 図形描画コマンドの引数の代わりにすることができる

```
cmark rm
```
- 一番新しい目印を削除する

```
chide
```
- カーソルを非表示にする

### 操作方法
- コマンドでカーソルを移動させることができる
- カーソルを移動させながらマークすることで図形描画に利用できる
	- 線を引く場合
		- csetとcmvでカーソルを引きたい線分の一端におく
		- cmarkで目印を置く
		- csetとcmvでもう一端に移動
		- line
	- 四角を描く場合
		- csetとcmvでカーソルを描きたい四角の左上に移動
		- cmarkで目印を置く
		- csetとcmvで四角の右下に移動
		- rect または fillrect
	- 円を描く場合(中心と半径を指定)
		- csetとcmvでカーソルを描きたい円の中心に移動
		- cmarkで目印を置く
		- csetとcmvで半径分移動する
		- circle または fillcircle
	- 円を描く場合(3点を通る円)
		- csetとcmvでカーソルを描きたい円の円周(どこでもよい)に移動
		- cmark
		- 同様に、もう一点cset, cmv, cmarkで目印を置く
		- さらに別の点にカーソルを移動させる
		- circle または fillcircle
	- 弧、または扇型を描く場合(3点を通る弧)
		- cset, cmv, cmarkで1点目(端)に目印を置く
		- 同様に2点目(端)に目印を置く
		- カーソルをマークした2点の間あたりにおく
		- sector または fillsector
- sample/yamaguchi.txt, sample/osaka_cursorで使われている

# プログラムについて
- レイヤー
	- レイヤーのリストは両方向連結リストで管理している
		- ただし、コーディングのしやすさからget_layerを使うことが多くprevは全く役に立っていない
		- get_layerをループ中に使うと実行時間がO(N^2)になるが、 Nは十分に小さいので今回は気にしていない
- 多角形
	- 内側を塗りつぶすとき多角形の内部かどうかの判定には[Crossing Number Algorithm](https://www.nttpc.co.jp/technology/number_algorithm.html)を用いた