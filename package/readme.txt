-------------------------------------------------------------------------------
Kuin Programming Language
v.2017.12.17
(C) Kuina-chan
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
1. はじめに
-------------------------------------------------------------------------------

    「くいなちゃん」が制作するプログラミング言語「Kuin」へようこそ。
    Kuinは、簡単で高速な実用プログラミング言語です。

    初めて触れる方は「チュートリアル( http://kuina.ch/kuin/a101 )」に沿ってお進
    めください。

    また、Kuinの詳細については「Kuinドキュメント( http://kuina.ch/kuin )」をご覧
    ください。
    「Kuinドキュメントは」、Kuinエディタのメニューから「ヘルプ > ヘルプの表示」
    をクリックするか、キーボードの「F1」キーを押しても開けます。

-------------------------------------------------------------------------------
2. 動作環境
-------------------------------------------------------------------------------

    KuinおよびKuinで制作したプログラムは、64bitのWindows7以降で動作します。

-------------------------------------------------------------------------------
3. 連絡先
-------------------------------------------------------------------------------

    あらゆるご質問、ご要望は「くいなちゃん」までどうぞ。

    Webサイト: http://kuina.ch/
    Twitterアカウント: https://twitter.com/kuina_ch (@kuina_ch)

    下記の投稿フォームからは、くいなちゃんに匿名で気軽にメッセージが送れますので
    ご活用ください。
    
    http://kuina.ch/others/contact_kuina

-------------------------------------------------------------------------------
4. 変更履歴
-------------------------------------------------------------------------------

v.2017.12.17
    - リリースビルド時に、resフォルダをres.kndファイルにアーカイブする機能の
      追加
    - HTTP通信を行うnet@Httpクラスの追加
    - Kuinエディタの操作感の改善
    - 細かな機能追加
      - lib@lerp、lib@qerp、lib@cerp、lib@hermite関数の追加
    - 細かな不具合の修正
      - draw@Font.drawで座標が小数の場合に線が描画されることがある不具合の修正

v.2017.11.17
    - ヘッダ情報が多すぎる.pngファイルの読み込みに失敗する不具合の修正
    - Kuinエディタに補完機能を追加
    - XMLライブラリ「xml」の追加
    - ネットワークライブラリ「net」の追加
    - 「excpt@invalidDataFormat」を「excpt@invalidDataFmt」に改名
    - 「excpt@noMemory」を「excpt@noMem」に改名
    - 「wnd@Wnd.setActive」を「wnd@Wnd.activate」に改名
    - 細かな機能追加
        - Kuinエディタの検索ダイアログの操作感の改善
        - lib@levenshtein関数の追加
        - list.sort、list.sortDesc、dict.toArrayKey、
          dict.toArrayValue、int.toStrFmt、float.toStrFmtメソッドの追加
    - 細かな不具合の修正
        - listの要素がcharの場合、toArrayの戻り値の末端に不正な文字が付加される
          不具合の修正
        - file@fileName関数の終端に文字コード0の文字が入る不具合の修正
        - []char.replaceメソッドのoldに空文字列を指定すると無限ループになる
          不具合の修正
        - kuin@Class.toStr()の戻り値をnullではなく空文字列が返るように修正
        - ドローコントロールのサイズを変更するとクリアカラーがリセットされる
          不具合の修正
        - float型を要素に持つ配列、list、stack、queue、dictの組み込みメソッドの
          うち、戻り値がfloatのメソッドが正しく動作しない不具合の修正
        - ダブルクリック時にwnd@Draw.onMouseDownLイベントが発生しない不具合の
          修正
        - file@Readerのファイル読み込みメソッドで2byte以上の文字が正しく読み込め
          ない不具合の修正
        - regex@Regex.replaceの書式がPerl互換になっていなかった不具合の修正

v.2017.10.17
    - 音が鳴らない環境でKuinエディタの起動に失敗する不具合の修正
    - 一部の環境やKuinエディタを多重起動したときにexeファイルの生成に失敗する
      不具合の修正
    - 数学・科学・アルゴリズムライブラリ「math」の追加
    - 正規表現ライブラリ「regex」の追加
    - 解像度の低い環境で大きいウインドウサイズを指定したときに、ウインドウが画面
      外に出ないように改善
    - Kuinエディタに「検索・置換」機能の追加
    - dbg@print関数の出力結果がKuinエディタ上で自動改行される不具合の修正
    - クラスのプロパティが関数型だった場合、関数呼び出しではなくメソッド呼び出し
      と解釈されてアクセス違反になる不具合の修正
    - 一部のライブラリで、デバッグ実行時にのみ発生する例外が発生しないことがある
      不具合の修正
    - 細かな機能追加
        - file@forEach関数の追加
        - 型の不一致のエラーメッセージで参照渡しを表示するように改善
        - lib@toRad、lib@toDegree、lib@makeBmSearch、lib@rndUuid、
          lib@cmpEx、lib@min、lib@minFloat、lib@max、lib@maxFloat関数の追加
        - int.sign、float.sign、lib@Rnd.rndUuid、[]char.findStr、
          []char.findStrLast、[]char.findStrEx、[].min、[].max、[].repeat、
          wnd@WndBase.getPosScrメソッドの追加
        - 「excpt@」以下に例外コードの定数を追加
        - kuin@ClassにtoStrメソッドを定義
    - 細かな不具合の修正
        - 実行直後の例外をKuinエディタで検知できない不具合の修正
        - Windows内部で発生する不明な例外をKuinエディタが検知してしまわないよう
          に修正
        - Kuinエディタ上でマウスホイールのスクロールを行ったときにスクロールバー
          が更新されない不具合の修正
        - 関数を超えてブロック変数にアクセスしたときに予期せぬエラーが発生する
          不具合の修正
        - input@setCfgKey関数で正しくキーが割り当てられない不具合の修正
        - []char.toInt、[]char.toFloatメソッドで変換の失敗が正しく検出できない
          不具合の修正

v.2017.9.17
    - 完成版としてリリース
