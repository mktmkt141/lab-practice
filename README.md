# ネットワーク・分散システム演習（Proxmox環境）

## 概要
このリポジトリは、研究室内で行ったネットワークおよび分散システムの演習課題の手順と設定を記録したものです。<br>
仮想マシンのスペックは以下の通りです。<br>
```conf
os:AlmaLinux

cpu
論理コア数: 8コア
仮想化環境: KVMによる仮想CPU (QEMUエミュレート)
ハイパースレッディング: 無効 (1スレッド/コア)

メモリ
物理メモリ総容量: 約 3.6 GiB

ディスク
ルートファイルシステム総容量：28GB
ブートファイルシステム総容量：960MB
```
## 目次
1. [Part 1:ネットワーク構築演習](#part-1ネットワーク構築演習)<br>
   ・[課題1:仮想ネットワーク構築](#-課題1仮想ネットワーク構築)<br>
   ・[課題2:NFS](#-課題2NFS)<br>
   ・[課題3:LDAP/SSSD　構築](#-課題3ldapsssd構築)<br>
2. [Part 2:分散システム構築](#part-2分散システム構築)<br>
   ・[課題1:MPI環境構築](#-課題1MPI環境構築)<br>
   ・[課題2:slurmクラスタ構築](#-課題2slurmクラスタ構築)<br>
   

---

## Part 1：ネットワーク構築演習

### 🔹 課題1：仮想ネットワーク構築
- VM0〜VM1 をクローンして構成
  vm0のipアドレス→192.168.20.229<br>
  vm1のipアドレス→192.168.20.230<br>
- VM間のpingテスト
- 外部ネットワークとの通信確認
- tracerouteを使って経路確認

**VM間のpingテスト**
(vm0のターミナルの上で)vm0→vm1の間の通信確認:`ping 192.168.20.230`<br>
(vm1のターミナルの上で)vm1→vm0の間の通信確認:`ping 192.168.20.229`<br>

**外部ネットワークとの通信確認**
(vm0,vm1の両方のターミナルの上で)`ping 8.8.8.8`

**tracerouteを使った経路確認**
`traecroute 8.8.8.8`

### 🔹 課題2：NFS
- vm0をサーバ、vm1をクライアントとしvm0て設定
- vm0での編集内容をvm1から見る
- vm1での編集内容をvm0から見る

**vm0(NFSサーバ)の設定**<br>
`sudo yum install nfs-utils -y`<br>
`sudo mkdir -p /home/admin/nfs_share`←/home/admin以下に共有するリポジトリを作成する。<br>
`sudo chmod -R 755 /home/admin/nfs_share`←権限を変更、rootユーザに対して読み取り、書き取り、実行を許可。rootグループに所属するユーザー、その他のユーザーに読み取りと実行を許可。<br>
`sudo nano /etc/exports`←nfsサーバがクライアントに共有するディレクトリの一覧を記述したファイルである「/etc/exports」に設定を記述する。<br>
```conf
# /etc/exports
/home/admin/nfs_share 192.168.20.230(rw,sync,no_root_squash)
```

`sudo systemctl enable --now nfs-server`←サーバの起動<br>
`sudo firewall-cmd --add-service=nfs --permanent`<br>
`sudo firewall-cmd --reload`←nfsサーバがファイアウォールにブロックされないように設定し、反映させる<br>
`echo "hello from 229" | sudo tee /home/admin/nfs_share/test.txt`←/home/admin/nfs_share/test.txtに「hello from 229」という内容を書き込み、標準出力に内容を表示させる<br>


**vm1(NFSクライアント)の設定**<br>
`sudo yum install nfs-utils -y`<br>
`sudo mkdir -p /mnt/nfs_mount`←/mnt/nfs_mountを作成<br>
`sudo mount -t nfs 192.168.20.229:/home/admin/nfs_share /mnt/nfs_mount`←vm0の/home/admin/nfs_share以下をvm1の/mnt/nfs_mountにマウントする<br>
`cat /mnt/nfs_mount/test.txt`←vm0で作ったtest.textの内容をvm1の上で見ることが出来た。<br>

**次は、vm1(クライアント)で作った内容をvm0(サーバ)でみられるようにする**<br>

**vm1の設定**<br>
`echo "Hello from client" | sudo tee /mnt/nfs_mount/hello.txt`←vm1で/mnt/nfs_mount/hello.txtに「Hello from client」と記述した<br>
**vm0の設定**<br>
`cat /home/admin/nfs_share/hello.txt`←vm1で作った内容をvm0の上で見ることが出来た<br>
![image](https://github.com/user-attachments/assets/575cfbcc-588f-46c8-84b9-7b2d5f1002c5)

### 🔹 課題3：LDAP/SSSD　構築

**LDAP設定**<br>
vm0(229)をldapサーバ、vm1(230)、vm2(201)、vm3(202)をldapクライアントとした。
#### OpenLdapサーバの構築<br>
`sudo dnf install epel-release`←epleリポジトリ（yum等にはないパッケージををインストールするためのサードパーティーリポジトリ)のインストールを行う<br>
`sudo yum -y install openldap*`←opneldap関連のパッケージをまとめてインストールするためのコマンド<br>
`sudo slappasswd`←ldap管理者用のパスワードをを暗号化形式で発行する<br>
`sudo nano chrootpw.ldif`←ldapの管理者のパスワードを設定、管理するファイルの編集<br>
`sudo ldapadd -Y EXTERNAL -H ldapi:/// -f chrootpw.ldif`←管理者パスワードの更新<br>
`sudo nano ldaproot.ldif`←openldapの設定情報に関するldifファイルを作成する<br>
ldaproot.ldifの中身はこちらです。<br>
'''conf
dn: olcDatabase={2}mdb,cn=config
changetype: modify
replace: olcSuffix
olcSuffix: dc=example,dc=com

dn: olcDatabase={2}mdb,cn=config
changetype: modify
replace: olcRootDN
olcRootDN: cn=admin,dc=example,dc=com

dn: olcDatabase={2}mdb,cn=config
changetype: modify
replace: olcRootPW
olcRootPW: {SSHA}X3n4mCAqsaDQAz5sMq6Hn8POsH4uLONm
'''
まずは、olcsuffixでldapディレクトリの検索ベースDN（ディレクトリルート）を指定する。dc=example、dc=comがルートになる。ここにldapに登録されるデータが集まる。olcrootdnで管理者dnの設定をする。ldap行う際のログインid的なものを作る。次に、olcrootpwでolcrootdnに対応するパスワードのハッシュを設定する。<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f ldaproot.ldif`←ldaproot.ldifファイルをldapサーバに反映させる<br>
続いて、自分のドメイン名の設定と、基本的なスキーマの読み込みを行う。<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/cosine.ldif`←古くからあるldapの基本属性定義<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/nis.ldif`←unixユーザの情報管理に必要な属性定義<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/inetorgperson.ldif`←一般的な人の情報を表すオブジェクトクラスの追加<br>
`sudo nano basedomain.ldif`<br>
basedomain.ldifの中身はこちらです。<br>
'''conf
dn: dc=example,dc=com
objectClass: top
objectClass: dcObject
objectClass: organization
o: Example
dc: example

dn: ou=People,dc=example,dc=com
objectClass: organizationalUnit
ou: People

dn: ou=Group,dc=example,dc=com
objectClass: organizationalUnit
ou: Group
'''
`sudo ldapadd -x -D "cn=admin,dc=example,dc=com" -W -f basedomain.ldif`←ldapのデータツリーのルートの定義、その反映<br>


以下の図が、ldapのディレクトリ構成図<br>
dc:ドメイン名の構成要素　ou:組織内のグループ、カテゴリを表す　dn:ldapのエントリを一意に識別するパス　cn:人、サービス、グループなどの名前(taro yamadaなど)
![image](https://github.com/user-attachments/assets/16225404-3713-48fd-9bc3-9a9f88a02a93)

ここまでがldapサーバの設定内容<br>

ここから、ssl/tlsの設定を行う。<br>
`sudo nano /etc/ssl/openssl.conf`←証明書生成時に使用する設定ファイルを編集する<br>
ここでは、証明書のsan(この証明書はどのホストで使えるかに関する設定)を設定した。←ここでは、どのホストでこの証明書を使えるかを定義している。また、dlp.example.comでというホスト名で証明書を指定している。<br>
`sudo openssl genrsa -aes128 -out /etc/pki/tls/certs/server.key 2048`←秘密鍵を作成し、`/etc/pki/tls/certs`に出力<br>
`sudo openssl rsa -in server.key -out server.key`←パスフレーズを除去した秘密鍵に変換する（ldapサーバ起動時にパスフレーズ入力を求められないようにするため）<br>
`sudo openssl req -utf8 -new -key server.key -out server.csr`←秘密鍵を使って、証明書署名要求（csrファイル）を作成する<br>
`sudo openssl x509 -in server.csr -out server.crt -req -signkey server.key -extfile /etc/ssl/openssl.cnf -extensions example.com -days 3650`←証明書の有効期限を10年に設定した。また、csrに対して自己署名し、サーバ証明書を作成する<br>
`sudo cp /etc/pki/tls/certs/{server.key,server.crt} /etc/openldap/certs/`←作成した秘密鍵を・証明書ldapサーバ専用のディレクトリ`/etc/openldap/certs/`に配置する<br>
`chown ldap:ldap /etc/openldap/certs/{server.key,server.crt}`←ファイルの所有者をldapユーザとグループに変更する（slapdが読み取れるようにするため）<br>
`nano mod_ssl.ldif`←ldapサーバにssl/tlsを有効化する設定を書いたldifファイルを作成する<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f mod_ssl.ldif`←mod_ssl/ldifを読み込んで、ssl/tlsを有効化する設定をサーバに反映させる<br>
`sudo firewall-cmd --add-service={ldap,ldaps}`←ファイアウォールの設定を行う。<br>

ここから、サーバにユーザーを登録していく。<br>
`sudo slappaswd`←ユーザのパスワードの設定を行う。<br>
`nano ldapuser.ldif`←ユーザー名をmktにして登録<br>
`sudo ldapadd -x -D "cn=admin,dc=example,dc=com" -W -f ldapuser.ldif`<br>
**ここでは、mktという名前のユーザーを登録した。**<br>


#### クライアントの設定（230,201,202側の設定）<br>
`sudo dnf -y install openldap-clients sssd sssd-ldap oddjob-mkhomedir`←ldapとsssdによる認証に必要なパッケージの追加。openldap-clientsとはldapsearchやldapaddなどldapサーバとやりとりするためのツール。sssdとは、ldapなどと連携して認証を行う仕組み。sssd-ldapとはsssdがldapと連携するためのモジュール。oddjob-mkhomedirとはログイン時に自動的にホームディレクトリを作るデーモン。<br>
`sudo authselect select sssd with-mkhomedir --force`←pam設定をsssdベースに切り替え、mkhomedirを有効にする。<br>
`sudo nano /etc/sssd/sssd.conf`←sssdがどのldapサーバを使うかの設定<br>
`sudo chmod 600 /etc/sssd/sssd.conf`←権限変更。<br>
`sudo systemctl restart sssd oddjobd`←sssdとoddjobdの再起動<br>
`sudo systemctl enable ssd oddjobd`←二つのデーモンの自動起動設定<br>


#### クライアントからmktでログインできるかを確認<br>
`su - mkt`←mktにユーザを変更<br>
全てのvmから同じコマンドでログインできるかを確認する。<br>
#### セキュアな通信が行えるかを確認する<br>
→「ldaps://~」でldapsearchできるかを確認する。<br>
何もしていない段階で、` ldapsearch -H ldaps://dlp.example.com -D "cn=admin,dc=example,dc=com" -W -b "dc=example,dc=com"`を実行すると、自己証明書をクライアント側に信頼させることと、名前解決ができていないため、エラーが吐かれる。<br>

そこで、229（サーバ側）で<br>
`scp /etc/openldap/certs/server.crt admin@192.168.20.230:/tmp/`←サーバにある証明書をクライアントへ転送している<br>
を打ち、次にクライアント側で<br>
`sudo mv /tmp/server.crt /etc/pki/ca-trust/source/anchors/ldap-server.crt`←受け取った証明書を信頼できる場所へ追加<br>
を打った。さらに、<br>
`sudo nano /etc/openldap/ldap.conf`←サーバ名や証明書の技術を行った<br>
を打ち、証明書の設定を記述した。ここでは、クライアントでサーバのssl/tls証明書を信頼させるための設定を行った。<br>
さらに、クライアント側の/etc/hostsファイルで以下のように設定を追加した。<br>

/etc/hostsに以下の行を追加した<br>
```conf
#/etc/hosts
192.168.20.229  dlp.example.com<br>
```

その後に<br>
` ldapsearch -H ldaps://dlp.example.com -D "cn=admin,dc=example,dc=com" -W -b "dc=example,dc=com"`<br>
を打ち、正常な出力が得られたので、セキュアな通信が上手くいったことが確認できた。↓結果<br>
```conf
# extended LDIF

# 

# LDAPv3

# base <dc=example,dc=com> with scope subtree

# filter: (objectclass=*)

# requesting: ALL

# 

# [example.com](http://example.com/)

dn: dc=example,dc=com
objectClass: top
objectClass: dcObject
objectClass: organization
o: Example
dc: example

# People, [example.com](http://example.com/)

dn: ou=People,dc=example,dc=com
objectClass: organizationalUnit
ou: People

# Group, [example.com](http://example.com/)

dn: ou=Group,dc=example,dc=com
objectClass: organizationalUnit
ou: Group

# mkt, People, [example.com](http://example.com/)

dn: uid=mkt,ou=People,dc=example,dc=com
objectClass: inetOrgPerson
objectClass: posixAccount
objectClass: shadowAccount
cn: mkt
sn: mkt
userPassword:: e1NTSEF9cnp1WTlXcjR2blhwcVNYa2Q0bkdlQWNRcDVINjUxVlU=
loginShell: /bin/bash
uidNumber: 2000
gidNumber: 2000
homeDirectory: /home/mkt
uid: mkt

# mkt, Group, [example.com](http://example.com/)

dn: cn=mkt,ou=Group,dc=example,dc=com
objectClass: posixGroup
cn: mkt
gidNumber: 2000
memberUid: mkt

# search result

search: 2
result: 0 Success

# numResponses: 6

# numEntries: 5
```
今までの設定を202、201のvmの中でも同様にする。<br>

#### 動機状態の確認<br>
サーバ（229）でパスワードの変更を行い、その変更がクライアント（201、202、230）で反映されているかを確認した。<br>
まずは以下のようにサーバ側でコマンドを打った。<br>
`ldappasswd -H ldap://localhost -x -D "cn=admin,dc=example,dc=com" -W -S "uid=mkt,ou=People,dc=example,dc=com"`<br>
ここで新しくパスワードを設定した。<br>
その後に、クライアント側から<br>
`su - mkt`<br>
このコマンドで新しいパスワードを使ってログインできるかを確認した。<br>


## Part 2：分散システム構築

### 🔹 課題1：MPI環境構築

この課題では、vm4台を用いて並列計算しています。まずは、各マシンでmpiのインストール、コンパイラの設定、サンプルプログラム(NFSを用いた並列計算)の実装を行います。<br>

**MPIインストール**

ローカルマシンから各vmにたいしてscpする。<br>
`sudo dnf install openmpi openmpi-devel -y`←openmpiとその開発用パッケージをインストール<br>
`echo 'export PATH=/usr/lib64/openmpi/bin:$PATH' >> ~/.bashrc`<br>
`echo 'export LD_LIBRARY_PATH=/usr/lib64/openmpi/lib:$LD_LIBRARY_PATH' >> ~/.bashrc`<br>
`source ~/.bashrc`<br>
上のコマンドでパスを通してください<br>

以下はdnfインストールできなかった時のためのコマンドで、自前ビルドです。おすすめしません。dnfでインストールした後に、パスを通してください<br>
[ここからインストール](https://www.open-mpi.org/)<br>
`tar -xvf openmpi-4.0.7.tar.gz`←解凍<br>
`sudo yum install -y perl`<br>
`sudo dnf install -y gcc-gfortran`←openmpiのビルドに必要なperlとfortran用のコンパイラをインストール<br>
`./configure --prefix=/usr/local/openmpi-4.0.7 CC=gcc CXX=g++ FC=gfortran`←インストール先を指定<br>
`make all`←ビルド<br>
`sudo make install`←インストール<br>
`sudo nano ~/.bashrc`←環境変数を設定<br>
一番下に以下の内容を追加する。パスを通す<br>
```conf
MPIROOT=/usr/local/openmpi-4.0.7
PATH=$MPIROOT/bin:$PATH
LD_LIBRARY_PATH=$MPIROOT/lib:$LD_LIBRARY_PATH
MANPATH=$MPIROOT/share/man:$MANPATH
```
そして、設定を変更。<br>
`source ~/.bashrc`←設定を反映<br>

このあとに、nfsの設定を行う。<br>
229のサーバで<br>
`sudo dnf install -y nfs-utils`←nfsサーバ用のツールをインストール<br>
`sudo mkdir DATA`<br>
`sudo chown admin:admin DATA` ←共有フォルダの作成と、所有者の変更<br>

`sudo nano /etc/exports`←nfsで共有するディレクトリの設定<br>
以下のように/etc/exportsを編集する。20.◯◯のクライアントに/home/admin/DATAの読み書き可能で共有する<br>
```conf
/DATA    192.168.20.0/24(rw,sync,no_subtree_check)
```

`sudo exportfs -ra`←設定の反映<br> 
`sudo exportfs -v `  ←確認<br>
`sudo systemctl enable --now nfs-server`←nfsサーバを起動、自動起動設定<br> 
`sudo firewall-cmd --permanent --zone=public --add-service=nfs`<br>
`sudo firewall-cmd --reload`←ファイアウォールのnfs許可<br>

他3台のクライアント側で<br>
`sudo dnf install -y nfs-utils`<br>
`sudo mkdir DATA`←クライアント用のツールのインストールと、マウント先の準備<br>
`sudo nano /etc/fstab`←nfsクライアント用のツールとマウント先を準備する<br>
以下のように/etc/fstabを編集する←サーバの/DATAをクライアントの/home/admin/DATAに設定する<br>
```conf
192.168.20.229:/DATA   /home/admin/DATA   nfs   defaults,_netdev   0  0
```
`sudo systemctl daemon-reexec`<br>
`sudo systemctl daemon-reload`<br>
`sudo mount /home/admin/DATA`←サービスの再読み込みと手動マウント<br>

マウントの設定を行う。<br>
この後に、229のサーバでパスワードなしでsshできるように以下の設定をする<br>
`ssh-keygen`<br>  # Enterキー連打でOK<br>
`ssh-copy-id admin@192.168.20.201`<br>
`ssh-copy-id admin@192.168.20.204`<br>
`ssh-copy-id admin@192.168.20.230`<br>
のようにパスワードなしで他のマシンに入れるようにする。<br>
この作業を他のマシンでも行う。<br>
各vmの/etc/hostsファイルの中で以下のように名前を設定する。<br>
```conf
192.168.20.229   vm0
192.168.20.201   vm1
192.168.20.204   vm2
192.168.20.230   vm3
```
さらに各vm◯で以下のコマンドを打つ。<br>
`sudo hostnamectl set-hostname vm◯`
また、hostfileで以下のように編集する。各マシンのプロセスを1に設定する<br>
```conf
192.168.20.229 slots=1
192.168.20.201 slots=1
192.168.20.204 slots=1
192.168.20.230 slots=1
```

ここでは、各マシンのプロセスを1に設定した。<br>
その後、コンパイする。<br>
`mpicc hello.c -o hello`<br>
[hello.cはここです](https://github.com/mktmkt141/lab-practice/blob/main/hello.c)<br>
`mpicc sum.c -o sum`<br>
[sum.cはここです](https://github.com/mktmkt141/lab-practice/blob/main/sum.c)<br>
この後にhello worldプログラムと、並列計算用のプログラムを実行する。<br>

`mpirun --hostfile hostfile ./hello`<br>
以下のように各マシンからの返答があれば成功。<br>
```conf
Hello World, I am 0 of 4 running on vm0
Hello World, I am 1 of 4 running on vm1
Hello World, I am 2 of 4 running on vm2
Hello World, I am 3 of 4 running on vm3
```
`mpirun --hostfile hostfile ./sum`<br>
以下のように各マシンから計算結果が返ってきたら成功。<br>
```conf
[プロセス0 - vm0] 4個のプロセスで協力して1から1000までの和を求めます！
[プロセス0 - vm0] 1から250までの和を計算します...
[プロセス0 - vm0] 1から250までの和は 31375 でした！
[プロセス2 - vm2] 501から750までの和を計算します...
[プロセス2 - vm2] 501から750までの和は 156375 でした！
[プロセス3 - vm3] 751から1000までの和を計算します...
[プロセス3 - vm3] 751から1000までの和は 218875 でした！
[プロセス1 - vm1] 251から500までの和を計算します...
[プロセス1 - vm1] 251から500までの和は 93875 でした！
[プロセス0 - vm0] 1から1000までの和は 500500 でした！
```


### 🔹 課題2：slurmクラスタ構築
vm0をマスターノード、vm0-vm3をワーカーノードとするクラスタの構築を行う。

vm0(229)で以下のコマンドを打つ。<br>
`sudo dnf install epel-release -y`（これは全てのvmで共通）←epelリポジトリの追加を行う<br>
`sudo dnf -y install slurm slurm-slurmd slurm-slurmctld munge`(これは全てのvmで実行）←スラームの基本パッケージとワーカーノードで動作するデーモン、マスターノードで動作するコントローラーデーモンをインストールする。また、ノード間で通信を行うための認証サービスmungeをインストールする。<br>
`sudo /usr/sbin/create-munge-key`←mungeの認証使用する秘密鍵の作成を行う。<br>
`sudo chown munge:munge /etc/munge/munge.key`←mungeの秘密鍵の所有者をとグループをmungeに変える。<br>
`sudo chmod 400 /etc/munge/munge.key`←ファイルの所有者にのみ読み取り権限を与える。<br>
次に、マスターノードの秘密鍵をワーカーノードの/tmpファイルにコピーする。<br>
`scp /etc/munge/munge.key admin@192.168.20.230:/tmp/`<br>
`scp /etc/munge/munge.key admin@192.168.20.201:/tmp/`<br>
`scp /etc/munge/munge.key admin@192.168.20.204:/tmp/`<br>
この後に、全てのノードで以下のコマンドを打つ。<br>
`sudo systemctl enable munge`←mungeサービスを全てのノードで自動起動するようにする。<br>
`sudo systemctl start munge`←mungeサービスの起動<br>
`sudo systemctl status munge`<br>

次に、マスターノード(229)の/etc/slurm/slurm.confの内容を以下のように記述する。<br>
```conf
# slurm.conf

# Cluster: cluster

# Master Node: vm0 (192.168.20.229)

# Worker Nodes: vm1 (192.168.20.201), vm2 (192.168.20.204), vm3 (192.168.20.230)

# --- General Parameters ---

ClusterName=cluster
SlurmctldHost=vm0 # マスターノードのホスト名
MpiDefault=pmix
# SlurmctldHost=backup\_controller\_hostname # バックアップコントローラがある場合は追記
#ControlMachine=vm0 # SlurmctldHostと同じマスターノードのホスト名を指定
#ControlAddr=192.168.20.229 # マスターノードのIPアドレス (ホスト名から解決できれば省略可)

SlurmUser=slurm
SlurmdUser=root

AuthType=auth/munge

# AuthInfo=/var/run/munge/munge.socket.2

# --- Logging and Directories ---

SlurmctldLogFile=/var/log/slurm/slurmctld.log
SlurmdLogFile=/var/log/slurm/slurmd.log
SlurmdSpoolDir=/var/spool/slurmd
StateSaveLocation=/var/spool/slurmctld
PluginDir=/usr/lib64/slurm

# --- Timers (秒単位) ---

InactiveLimit=0
KillWait=30
MinJobAge=300
WaitTime=0

# --- Scheduling ---

SchedulerType=sched/backfill
SelectType=select/cons\_tres
SelectTypeParameters=CR\_Core\_Memory

# --- Job Management ---

FirstJobId=1
MaxJobCount=50000
JobCompType=jobcomp/none
JobCompLoc=/var/log/slurm_jobcomp.txt

# --- Ports ---

SlurmctldPort=6817
SlurmdPort=6818

# --- Process Tracking ---

ProctrackType=proctrack/cgroup
TaskPlugin=task/cgroup,task/affinity

# --- Node Definitions ---

# CPUコア数: 各ノード 8 コア

# RealMemory: 各ノード 3.6GiB (約3686MB) のうち、3500MB をジョブ利用可能メモリとして設定

# (3686MB - OS等予約分186MB = 3500MB で計算。この値は調整可能です)

NodeName=vm0 NodeAddr=192.168.20.229 CPUs=8 RealMemory=3500 State=UNKNOWN
NodeName=vm1 NodeAddr=192.168.20.201 CPUs=8 RealMemory=3500 State=UNKNOWN
NodeName=vm2 NodeAddr=192.168.20.204 CPUs=8 RealMemory=3500 State=UNKNOWN
NodeName=vm3 NodeAddr=192.168.20.230 CPUs=8 RealMemory=3500 State=UNKNOWN

# --- Partition Configuration ---

# パーティション名を 'mycluster' とし、ノードを指定します。

# マスターノード(vm0)を計算リソースとして使用する場合:

PartitionName=mycluster Nodes=vm0,vm1,vm2,vm3 Default=YES MaxTime=INFINITE State=UP


# --- オプション: Epilog と Prolog スクリプト ---

# Epilog=/etc/slurm/epilog.sh

# Prolog=/etc/slurm/prolog.sh

```

次に、全てのノードで以下のように編集する。<br>
`sudo useradd -m slurm`←新しいユーザーslurmを/home/slurmに追加する。<br>
`sudo mkdir -p /var/spool/slurmctld /var/spool/slurmd /var/log/slurm`←スラームコントローラとスラームデーモンが一時的なデータを保存するディレクトリを作る。また、スラーム関連のログファイルを保存するためのディレクトリを作る。<br>
`sudo chown slurm:slurm /var/spool/slurmctld /var/spool/slurmd /var/log/slurm`←上で作ったディレクトリの所有者とグループの変更を行う。<br>

次に、マスターノードで以下のコマンドを打つ。スラーム設定ファイルをワーカーノードにコピーする。<br>
`scp /etc/slurm/slurm.conf admin@192.168.20.230:/etc/slurm/`<br>
`scp /etc/slurm/slurm.conf admin@192.168.20.201:/etc/slurm/`<br>
`scp /etc/slurm/slurm.conf admin@192.168.20.204:/etc/slurm/`<br>
スラームコントローラサービスの自動起動設定と、起動を行う。<br>
`sudo systemctl enable slurmctld`<br>
`sudo systemctl start slurmctld`<br>

全てのノードで以下のようにコマンドを打つ。ワーカーノードでスラームデーモンサービスの自動起動と起動を行う。<br>
`sudo systemctl enable slurmd`<br>
`sudo systemctl start slurmd`<br>

マスターノードでスラームクラスタの情報を確認するためのコマンドを打つ。「idle」と表示されていればよい。<br>
`sinfo`<br>
以下のように出力されていればよい。<br>
```conf
PARTITION  AVAIL  TIMELIMIT  NODES  STATE NODELIST
mycluster*    up   infinite      4   idle vm[0-3]
```

続いて、hostnameプログラムを実行する。[このシェルスクリプト](https://github.com/mktmkt141/lab-practice/blob/main/hostname.sh)をもとにホスト名を出力するジョブを投げる。出力されたファイルにホスト名が表示されているかを確認する。<br>
マスターノードで以下のように出力されればよい。<br>
`sbatch hostname.sh`←
```conf
[admin@vm0 ~]$ cat slurm-11.out
vm0
vm3
vm2
vm1
```
続いて、[このシェルスクリプト](https://github.com/mktmkt141/lab-practice/blob/main/mpi.sh)をもとにMPIを実行するジョブを投げ、出力されたファイルに期待される結果が書き込まれているかを確認する。<br>
`sbatch mpi.sh`←バッチジョブをキューに投下する<br>
```conf
[admin@vm0 ~]$ cat slurm-26.out
Hello World from vm3, I am rank 3 of 4
Hello World from vm2, I am rank 2 of 4
Hello World from vm0, I am rank 0 of 4
Hello World from vm1, I am rank 1 of 4
```
このように各プロセスからの出力が表示されていれば良い。<br>








