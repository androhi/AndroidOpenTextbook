= Androidアプリのバックグラウンド

本章では、Androidアプリケーションの非同期処理、バックグラウンド処理を解説します。
UIスレッドで時間のかかる処理を行うと、その処理が終わるまでユーザの操作を受け付けなくなります。Androidには時間のかかる処理、たとえばデータ通信やストレージアクセスなどを非同期で行う仕組みがあります。
また常駐型アプリケーションではServiceを使い、バックグラウンドで処理を行えます。
ここではアプリケーションを作るために、Androidの持つ非同期通信、バックグラウンド処理の概要を理解したあと、AsyncTaskやServiceなど実際のAPIについて学んでいきましょう。

ゴールは、非同期処理、バックグラウンド処理について独力で最適な実装を選択できるようになることです@<fn>{最適解}。
ひとくちに最適な手法といっても、アプリケーションの仕様や開発者の実力、プラットフォームの互換性など多くの要因に左右され、解はひとつではありません。誰かが「この手法が最も良かった」と言ったからといって自分にとっても最適であるか、わからないためです。各手法の特徴を学び、設計にあった取捨選択を目指します。

//footnote[最適解][ソフトウェア開発において共通した要素があり、プログラミングでの定石をデザインパターンとよびます。開発における設計ノウハウをまとめたものとしては、書籍『オブジェクト指向における再利用のためのデザインパターン』が特に有名です]

== 非同期処理とバックグラウンド
はじめに、本章で取り扱う機能やメソッドについて特徴を紹介します。


#@# 全部かいてあとで戻ってきて書こう。

=== フォアグラウンドとバックグラウンド
フォアグラウンドとは、ユーザーインターフェイス（UI）や操作できる画面や処理、状態を指します。
バックグラウンドとは、ユーザーにみせるUIの裏側で動作し、ユーザーに意識させずに行われる処理、状態です。バックグラウンドではUIを持たない性質上、ユーザーとの対話なしに行えるタスク（処理）に向いています。

AndroidではActivityがフォアグラウンド処理を、Serviceがバックグラウンド処理を担当します。バックグラウンド処理を持つAndroidアプリケーションとしては、Musicアプリなどが代表的です。

現在のコンピュータの多くは複数の作業を切り替えて実行するマルチタスク環境を持っており、それはAndroidも同様です。
コンピュータのCPUやI/O（ファイル入出力）など限りあるリソースを効率的に利用するために、マルチタスク環境では待ち時間の合間に他のタスクを実行しています。
一般的にフォアグラウンド処理はUIを担当するため、バックグラウンド処理より優先度が高く設定されています。


=== プロセスとスレッド
Androidアプリケーションではプロセスはアプリケーションごとの実行単位、実行環境です。基本的に1つのアプリケーションに対して1つのプロセスが割りあてられます。
アプリを実行するために必要なリソース（メモリやCPUなど）は、プロセスごとに管理されています。
またプロセスは新しいプロセス（子プロセス）を生成できます。子プロセスは、生成元となったプロセス（親プロセス）とは異なるリソースを利用します。
このため、複数のプロセスで同じメモリ空間を読み書きするといったリソースの共有はできません。

プログラムが動く仕組みを理解するうえで重要な考え方です。

スレッドとは処理の単位のことです。プロセスAndroidのActivityに関する処理は通常mainスレッドで動作しています。
onCreateやonPauseメソッド、各UIパーツ（ボタン、Viewなど）の描画処理はmainスレッドで動いています。


====[column] デーモンスレッドとユーザースレッド
スレッドの種類は2つに分類されます。デーモンスレッドとは、プログラム終了時にスレッドの実行終了を待ちません。
プログラム終了のタイミングでデーモンスレッドの処理は中断され、終わることになります。生成したスレッドで終了処理を意識しないで済むので使い勝手がよい側面があります。

ユーザースレッドは、デーモンスレッドの反対でプログラムを終わるときに、スレッドの実行終了を待ちます。プログラムを終了しようとしても、
ユーザースレッドの処理が終わるまで終了できません（ユーザースレッドが生き残っている間は、プログラム実行状態です）。
さきほど説明にでてきた、UIスレッドはユーザースレッドです。プログラム終了にあたっては、すべてのユーザースレッドできちんと終了処理がハンドリングできていないといけません。
利点として、処理が中断してしまう恐れはないので紛失できない大事な処理などに適しています。
====[/column]


=== 同期と非同期処理
同期と非同期は、複数の処理（スレッド）を同時に行う上で重要な考え方です。複数のスレッドを並行的に処理することをマルチスレッドと呼びます。同期処理とは同一スレッド上での動作を指します。また複数のスレッドが処理を待ち合わせることを同期する、と表現します。

非同期処理では、複数のスレッドが阻害し合うことなく、独立して処理を行います。UIスレッドで行うには時間がかかりすぎる場合、たとえばネットワーク通信、ディスクアクセス、画像処理などは、積極的に非同期化すべきです。本来の役割であるUI描画をブロックしてしまい、画面が固まったようにみえるためです。


=== UIスレッドをブロックしない

Androidではユーザーの操作は、UIスレッドで処理されます@<fn>{single}。ボタンやテキストなどUIコンポーネントのインスタンスは、すべてUIスレッドで生成、操作しています。たとえばボタンを押したときに発生するonClick、画面をタッチするときに発生するonTouchEventでは、UIスレッドで実行しています。

//footnote[single][シングルスレッドモデル]

これらのメソッドで時間のかかる処理を行った場合、UIスレッドの他の処理はブロックされ、見た目に変化がなく固まった状態となります。
一般的には、無応答時間（処理時間）が100ms〜200msを超えると応答性が悪いと感じるため、
処理時間は非同期化を検討する指標のひとつといえるでしょう。

Androidアプリ開発では、原則として次の2つを守ってください。

 * UIスレッドで時間のかかる処理をしない
 * UIスレッド以外からUIコンポーネントを操作しない

特にアプリケーションが5秒以内に応答しなかった場合を、ANR（Application Not Responding）と呼び、次のようなダイアログが表示されます。

#@# イメージ

ANRは、アプリケーションの異常を検出してユーザーが操作に困るような事態を防ぐ機構です。Android OSは、アプリケーションの挙動を常時監視しています。
５秒以上反応がないということは、アプリそのものが異常な状態（フリーズ、ハングアップ）ですので、このような状況は開発者として絶対に避けるべきです。

=== ANRを発生させる要因

では、ANRが起きる、というのはどういう状況なのでしょうか。ここではサンプルコードを使って実際にANRを発生させてみましょう。次の３つは、時間がかかる処理のなかでも代表的な処理です。

 * CPUによる演算
 * ストレージアクセス
 * ネットワーク通信

順番にどのようなコードがANRを引き起こすのか確認してみましょう。

はじめのCPUによる演算とは、目に見えて処理時間がかかるほどの膨大な計算量が要因です。

#@# サンプルコード

サンプルコードではループ回数が大きく、非常に時間がかかっています。サンプルのような簡単なソースコードでは、時間がかかることも一目瞭然です。一般に、複雑なコードになればなるほど処理時間は、延びていきます。

アプリケーション開発において、すべての処理について理解し、把握していることが望ましいですが、実際には用意されたAPIを使ったり、ライブラリを使ったり、と多くの要素が絡み合っています。開発者の意図どおり、遅滞無く実行されているかは、処理時間を計測していくことでのみ確認できます。ANRの予防には計測が重要です。

つぎに、ストレージアクセスを見てみましょう。ストレージは設定値の保持や画像の一時保存などで利用されています。Android端末内のメモリストレージにデータを読み書きする場合、UIスレッドで処理しないほうがよいケースがあります。

#@# サンプルコード

サンプルコードでは大量のデータを一気に書き込もうとしています。一般に書込みは読込みより時間がかかり、ストレージアクセス開始から終了まで操作ができなくなります。

それでは、このようなケースではどうでしょうか。

#@# サンプルコード

サンプルコードでは、ごくわずかな時間で処理が完了しています。この場合、別スレッドを用意する価値はどの程度でしょう。応答速度の観点からみるとUIスレッドで処理しても良いと言えます@<fn>{異論}。

//footnote[異論][もちろん他スレッドで実行するほうが好ましいです。しかし、処理が複雑になると不具合が混入する可能性も増えます。分かりにくいソースコードになってしまうことも本意ではありません]

最後に、ネットワーク通信です。AndroidではUIスレッドでネットワーク通信を行った場合、エラーが発生します。ネットワーク通信は、非常に時間がかかるためで、UIスレッドでの実行が禁止されています。ストレージアクセスと扱いが異なる理由は、ネットワーク通信がすぐに応答が来る 

#@# TODO 

#@# サンプルコード


#@# logcatのエラー

=== Androidアプリはどうやって同時に動作しているのか？

Androidでは複数のアプリケーションを同時に実行できます。原則としてActivityは1つしか許可されませんが、バックグラウンドでServiceが動作しています。
本文中の言葉で言い換えると、Androidはマルチプロセス動作のための機構を持っているわけです。
そのための機構はActivityManagerと呼ばれ、ユーザーへのインタラクションを管理しています。

ActivityManagerは、アプリケーションを管理するため、次のようなことを行っています。

 * ライフサイクルに応じてActivity／Serviceを管理する
 * 優先度に応じてプロセスを制御する
 * 未使用のプロセスを終了させてメモリ空間を確保する

利用者がストレスを感じないように調停し、場合によっては未使用のプロセス（アプリ）を終了します。たとえば利用中のアプリがより多くのメモリを要求した場合、
バックグラウンドに存在しているアプリを終了し、利用できるメモリを増やします。

優先度についても良好な応答性が維持できるように制御されています。
フォアグラウンドのActivityは、ユーザーへのインタラクションを提供しているため、最も優先度が高いプロセスとして制御します。
またバックグラウンドにあるServiceもメモリに余裕がある限り動作し続けられます。言い換えるとメモリがひっ迫した場合は、Serviceであっても終了させられます。

おおまかに優先度は次のとおりです。

 * フォアグラウンドのActivity
 * フォアグラウンドのService@<fn>{startForeground}
 * バックグラウンドのService
 * バックグラウンドのActivity

//footnote[startForeground][ServiceクラスのstartForegroundメソッドをつかい、通知バーに常駐することでフォアグラウンド動作が可能]

システムによる終了は低メモリ時に行われ、外部要因に依存します。これはOSが高負荷状態であるなど機種の状態や、他のアプリケーションの影響を受けることを示唆しています。
フレームワークの挙動を理解してアプリの設計段階から考慮しておくべきでしょう。

== 基本的な非同期処理
Androidフレームワークには、非同期処理のための機能が用意されています。ここでは良く用いられる非同期通信の方法について概要を解説します。実装、用法については後述の節を参考にしてください。

 * AsyncTask

一回限りの非同期処理に向いています。Androidフレームワークの非同期用クラスです。UIスレッドからしか生成できません。非同期処理には、スレッドの生成や破棄といった作業は不可欠ですが、煩雑です。AsyncTaskクラスは、このようなスレッド管理をフレームワーク内部で完結しています。

 * AsyncTaskLoader

複数回繰り返される非同期処理に向いています。AsyncTaskクラスを効率的に扱うためのLoaderクラスです。Loaderとは、AsyncTaskを読み込む（ロードする）ためのクラスです。高速化やリソースの効率化の仕組みとしてキャッシュを持っています。複数回繰り返す処理に向いています。UIスレッド以外からも生成できます。

 * Thread

Javaで提供されているクラスです。もっとも基本的なクラスで、スレッドの生成、スリープ、実行など基本的な操作ができます。ただし、Androidでは描画はUIスレッドでしか行えないため、処理結果をUIに反映する場合はHandlerを理解して利用しなければいけません。自由度が高い一方、実装コストも高くなります。

 * ExecutorService

Javaで提供されているクラスです。ExecutorServiceの実装は目的に応じて複数用意されており、単一のスレッドでの順次実行、複数のスレッドを使い分けられます。処理内容、CPUやメモリなどのリソースに応じて、どの実装（提供されたクラス）を使うか、取捨選択が必要です。

 * Handler

Handlerクラスは、処理順序のスケジューリングや、処理を別スレッドで実行するための機能を持っています。
特にUIスレッドで描画処理を実行するために利用します。

 * Service

バックグラウンド動作のためのクラスです。音楽再生のようにバックグラウンドで継続して処理を行うために利用します。
スレッドとは異なります。

 * IntentService

 IntentServiceはアプリケーションの動作状況に依存しないで、自分の仕事がなくなるまでバックグラウンドで処理を行います。AsyncTaskと似ていますがアプリケーションがフォアグラウンドでなくなっても中断されない利点があります。


=== 良い実装のための指標
良い設計の第一歩は、各手法の特徴を理解しておくことです。めやすとしては次の通りです。

 * １回限りの処理、または繰り返しの少ない処理：AsyncTask
 * リスト表示するなど頻出する繰り返し処理：AsyncTaskLoader
 * 実行完了の保証が欲しい処理：IntentService
 * アプリ終了後もバックグラウンド動作したい処理：Service

プログラミングの目的は、さまざまなので、すべてのケースに当てはまるわけではありませんが、基本として理解しておくと応用の幅がひろがります。


== AsyncTaskとLoader

AsyncTaskは非同期処理のための便利なヘルパークラスです。AsyncTaskクラス内部では、
非同期処理の為にThreadとHendlerが使われていますが、クラス内で隠蔽されており意識する必要はありません。

AsyncTaskクラスは、非同期処理を４つのステップに区切っています。実行前、実行開始、実行中、実行後です。それぞれonPreExecuteメソッド、doInBackgroundメソッド、onProgressUpdateメソッド、onPostExecuteメソッドが担当します。

#@# 絵

AsyncTaskクラスを使う際には次の特徴に注意してください。

 * UIスレッドでインスタンスを生成する
 * 実行済みのインスタンスは再利用できない
 * onPostExecuteメソッドはUIスレッドで実行される

AsyncTaskクラス最大の利点は簡単に非同期処理できるところです。そのための制約としていくつか制限があり、そのうちの1つがUIスレッドでインスタンスを生成することです。もうひとつは実行済みのインスタンスは再利用できません。つまり、１度実行したAsyncTaskは再度実行できません。破棄するのみです。

再利用性が低い設計がになっていることからも、一度限りの処理に向いている（そのために作られた）といえます。
そしてもっとも嬉しい点は、onPostExecuteメソッドはUIスレッドで実行されることでしょう。

通常ならUIコンポーネントを操作するためには、Handlerを使ってUIスレッドで処理するコードが必要です。
AsyncTaskならそのような面倒なコードは不要というわけです。

#@# ジェネリクスの話

=== AsyncTaskを使わない場合

ここでは非同期処理が必要な計算量の多い例として、画像処理を扱います。
Play Storeにもトイカメラ風の写真を撮るアプリなど面白い効果をもったカメラアプリがいくつも公開されていますが、
今回はシンプルに用意されている画像をカラーからモノクロに変換するアプリを作ってみましょう。

#@# イメージ

画面下の開始ボタンを押すと画像をモノクロに変換する処理を行います。

まずは、AsyncTaskを使わずにプログラミングしてみます。これは、計算量の多い処理があるとUIスレッドが止まり、
ユーザーが操作できないことを確認するためです。操作できないことを体験するために、２番目のボタンも用意します。
このボタンを押すと表示されているラベル（数字）がカウントアップしていくものです。

#@# スケルトンXML

#@# スケルトンプログラム

アプリを実行して、開始ボタンとカウントボタンを押してみましょう。しばらく待っていると画像が、カラーからモノクロへ変換されます。
すべてUIスレッドで処理しているため、開始ボタンを押したあとはカウントボタンが操作できず、最悪の場合、ANRに陥ります@<fn>{連打}。

//footnote[連打][ANRを確認するために、カウントボタンを連続で押してみてください]

=== AsyncTaskを使う場合

前述したサンプルコードを変更して、画像処理部分を分離し、非同期化します。
このサンプルコードでは画像処理という、比較的わかりやすい例を取り扱っています。実際のアプリ開発においては、
重い処理を探し出して非同期化することになります。
サンプルは処理を非同期化するスタンダードな手法として覚えておくとよいでしょう。

AsyncTaskの主なメソッドは次の通りです

 * onPreExecute()：事前に準備する内容を記述する
 * doInBackground(Params...)：バックグラウンドで実行する
 * onProgressUpdate(Progress...)：進捗状況をUIスレッドで表示する
 * onPostExecute(Result)：バックグラウンド処理が完了し、UIスレッドに反映する

各メソッドのParams、Progress、Resultは引数となるクラスの例です。実際には必要に応じて指定します。


#@# サンプルコード

ここで注目してもらいところは 「extendsAsyncTask<Bitmap, void, Bitmap>」です。前述の引数ではParams、Progress、Resultが出てきましたが、ここで引数の型を指定しています。

１番目のParamsはバックグラウンド処理を実行する時に与えるexecuteメソッドの引数の型です。
２番目のProgressは進捗状況を表示するonProgressUpdateメソッドの引数の型です。
最後のResultはバックグラウンド処理の後に受け取るonPostExecuteメソッドの引数の型です。

今回はビットマップ画像を与えてモノクロに変換されたビットマップを受け取ります。
まずは進捗表示を行わないのでProgressにはVoid（引数を持たない意味の型）を指定しています。


Activity側で必要な処理は、MonochromeTaskクラス（Asynctaskのサブクラス）のインスタンスを生成することと、
非同期処理を開始するために、executeメソッドを呼び出すことです。

#@# サンプルコード

Asynctaskを使わない場合と違い、UIスレッドをブロックしません。
この状態でカウントボタンを押すと、問題なくカウントアップされていきます。



== Thread、ExecutorService


== Handler


== Service


== IntentService


== プロセス間通信（AIDL）


== Androidでの優先度制御


== デバッグツール

systraceは とTraceview、ストリクトモード
