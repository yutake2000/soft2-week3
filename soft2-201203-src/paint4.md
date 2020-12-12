# ファイル構成
- paint4.c
	- メインのプログラム
- paint4.h
	- 関数・構造体の定義を行うヘッダファイル
- layer.c
	- レイヤー操作の関数が多いためpaint4.cから分割した
	- paint4.cでincludeしている

- sample(フォルダ)
	- load, animateコマンドで読み込めるファイルが入っている

# 実行方法

## コンパイル
```
gcc -o paint4 paint4.c
```

## 実行
```
paint4 <width> <height> [pallet]

例:
paint4 40 40 pallet
```
- サンプルは基本40×40マスを想定している
- palletオプション
	- これを入れるとキャンバスの右側にパレットが表示される
	- colorコマンドでは色を数字で指定するので、このパレットに書かれて数字を使う

## サンプル
```
paint4 40 40
```
- 上のコマンドをターミナルで実行した後、下記のファイルを読み込むコマンドをプログラム中で入力する
- 入力後はEnterキーを押すごとにコマンドが1つずつ実行されていく
- 1つのファイルが終了したら次のanimateコマンドを実行する

```
animate sample/osaka.txt
animate sample/nagasaki.txt
animate sample/google_photo.txt
animate sample/google_play.txt
```

# コマンド一覧

## 描画
```
line <x0> <y0> <x1> <y1>
```
- (x0, y0)から(x1, y1)まで線分を描画する

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
```
- (x0, y0)を中心とする半径r, 角度θ1からθ2までの弧を描画する
- θ1・θ2はx軸方向を0度とした反時計周りの角度
- arcとsectorのような気もするが、他のコマンドと合わせた

```
polygon <x0> <y0> <x1> <y1> ...
fillpolygon <x0> <y0> <x1> <y1> ...
```
- (x0, y0), (x1, y1), ... (xn, yn)を頂点とする多角形を描画する

## ツール
```
chpen <pen>
```
- ペンをpen(1文字)に変更する

```
color <color code>
```
- 色を変更する(0 - 255)


```
marker
```
- ペンをマーカーに変更する
- 戻す場合はchpenコマンドを用いる

```
pensize <size>
```
- ペンの太さを変更する
- デフォルトは1
- 線や多角形の場合は(2 * size + 1)ピクセル、円の場合はsizeピクセル程度になる

```
eraser
```
- ペンを消しゴムに変更する
- 戻す場合はchpenコマンドを用いる

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

## キャンバス操作

```
resize <width> <height>
```
- 盤面のサイズを変更する

```
aspect <ratio>
```
- アスペクト比を変更する
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