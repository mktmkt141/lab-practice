# ネットワーク・分散システム演習（Proxmox環境）

## 概要
このリポジトリは、研究室内で行ったネットワークおよび分散システムの演習課題の手順と設定を記録したものです。仮想マシンは Proxmox 上で構築しました。

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
`sudo nano chrootpw.ldif`<br>←ldapの管理者のパスワードを設定、管理するファイルの編集<br>
`sudo ldapadd -Y EXTERNAL -H ldapi:/// -f chrootpw.ldif`←管理者パスワードの更新<br>
`sudo nano ldaproot.ldif`←openldapの設定情報に関するldifファイルを作成する<br>
ldaproot.ldifの中身はこちらです。<br>
まずは、olcsuffixでldapディレクトリの検索ベースDN（ディレクトリルート）を指定する。dc=example、dc=comがルートになる。ここにldapに登録されるデータが集まる。olcrootdnで管理者dnの設定をする。ldap行う際のログインid的なものを作る。次に、olcrootpwでolcrootdnに対応するパスワードのハッシュを設定する。<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f ldaproot.ldif`←ldaproot.ldifファイルをldapサーバに反映させる<br>
続いて、自分のドメイン名の設定と、基本的なスキーマの読み込みを行う。<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/cosine.ldif`←古くからあるldapの基本属性定義<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/nis.ldif`←unixユーザの情報管理に必要な属性定義<br>
`ldapadd -Y EXTERNAL -H ldapi:/// -f /etc/openldap/schema/inetorgperson.ldif`←一般的な人の情報を表すオブジェクトクラスの追加<br>
`sudo nano basedomain.ldif`<br>
`sudo ldapadd -x -D "cn=admin,dc=example,dc=com" -W -f basedomain.ldif`←ldapのデータツリーのルートの定義、その反映<br>


以下の図が、ldapのディレクトリ構成図<br>
dc:ドメイン名の構成要素　ou:組織内のグループ、カテゴリを表す　dn:ldapのエントリを一意に識別するパス　cn:人、サービス、グループなどの名前(taro yamadaなど)
![image](https://github.com/user-attachments/assets/16225404-3713-48fd-9bc3-9a9f88a02a93)

ここまでがldapサーバの設定内容<br>

ここから、ssl/tlsの設定を行う。<br>
`sudo nano /etc/ssl/openssl.conf`←証明書生成時に使用する設定ファイルを編集する<br>
ここでは、証明書のsan(この証明書はどのホストで使えるかに関する設定)を設定した。←ここでは、どのホストでこの証明書を使えるかを定義している。また、dlp.example.comでというホスト名で証明書を指定している。
`sudo openssl genrsa -aes128 -out /etc/pki/tls/certs/server.key 2048`←秘密鍵を作成し、`/etc/pki/tls/certs`に出力<br>
`sudo openssl rsa -in server.key -out server.key`←パスフレーズを除去した秘密鍵に変換する（ldapサーバ起動時にパスフレーズ入力を求められないようにするため）<br>
`sudo openssl req -utf8 -new -key server.key -out server.csr`←秘密鍵を使って、証明書署名要求（csrファイル）を作成する<br>
`sudo openssl x509 -in server.csr -out server.crt -req -signkey server.key -extfile /etc/ssl/openssl.cnf -extensions example.com -days 3650`←証明書の有効期限を10年に設定した。また、csrに対して自己署名し、サーバ証明書を作成する<br>
`sudo cp /etc/pki/tls/certs/{server.key,server.crt} /etc/openldap/certs/`←作成した秘密鍵を・証明書ldapサーバ専用のディレクトリ`/etc/openldap/certs/`に配置する<br>
`chown ldap:ldap /etc/openldap/certs/{server.key,server.crt}`←ファイルの所有者をldapユーザとグループに変更する（slapdが読み取れるようにするため）<br>
`nano mod_ssl.ldif`←ldapサーバにssl/tlsを有効化する設定を書いたldifファイルを作成する<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f mod_ssl.ldif`←mod_ssl/ldifを読み込んで、ssl/tlsを有効化する設定をサーバに反映させる<br>
`sudo firewall-cmd --add-service={ldap,ldaps}`←ファイアウォールの設定を行う。

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
→「ldaps://~」でldapsearcできるかを確認する。<br>
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
192.168.20.229  dlp.example.com<br>

その後に<br>
` ldapsearch -H ldaps://dlp.example.com -D "cn=admin,dc=example,dc=com" -W -b "dc=example,dc=com"`<br?
を打ち、正常な出力が得られたので、セキュアな通信が上手くいったことが確認できた。<br>
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
[ここからインストール](https://www.open-mpi.org/)<br>
ローカルマシンから各vmにたいしてscpする。<br>
`sudo dnf install openmpi openmpi-devel -y`
`tar -xvf openmpi-4.0.7.tar.gz`
`sudo yum install -y perl`
`sudo dnf install -y gcc-gfortran`
`./configure --prefix=/usr/local/openmpi-4.0.7 CC=gcc CXX=g++ FC=gfortran`
`make all`
`sudo make install`
`sudo nano ~/.bashrc`
一番下に以下の内容を追加する
export PATH=/usr/lib64/openmpi/bin:$PATH
export LD_LIBRARY_PATH=/usr/lib64/openmpi/lib:$LD_LIBRARY_PATH
MPIROOT=/usr/local/openmpi-4.0.7
PATH=$MPIROOT/bin:$PATH
LD_LIBRARY_PATH=$MPIROOT/lib:$LD_LIBRARY_PATH
MANPATH=$MPIROOT/share/man:$MANPATH
export MPIROOT PATH LD_LIBRARY_PATH MANPATH
そして、設定を変更。<br>
`source ~/.bashrc`

このあとに、nfsの設定を行う。<br>
229のサーバで<br>
`sudo dnf install -y nfs-utils`<br>
`sudo mkdir DATA`<br>
`sudo chown admin:admin DATA` <br>

`sudo nano /etc/exports`<br>
`/home/admin/DATA    192.168.20.0/24(rw,sync,no_subtree_check)`<br>

`sudo exportfs -ra`  <br>
`sudo exportfs -v `  <br>
`sudo systemctl enable --now nfs-server` <br>
`sudo firewall-cmd --permanent --zone=public --add-service=nfs`<br>
`sudo firewall-cmd --reload`<br>

他3台のクライアント側で<br>
`sudo dnf install -y nfs-utils`<br>
`sudo mkdir DATA`<br>
`sudo nano /etc/fstab`<br>
192.168.20.229:/DATA   /DATA   nfs   defaults,_netdev   0  0
192.168.20.229:/home/admin/DATA   /DATA   nfs   defaults,_netdev   0  0
`sudo systemctl daemon-reexec`<br>
`sudo systemctl daemon-reload`<br>
`sudo mount /DATA`<br>

マウントの設定を行う。<br>
この後に、229のサーバで<br>
`ssh-keygen`<br>  # Enterキー連打でOK
`ssh-copy-id admin@192.168.20.201`<br>
`ssh-copy-id admin@192.168.20.204`<br>
`ssh-copy-id admin@192.168.20.230`<br>
のようにパスワードなしで他のマシンに入れるようにする。<br>
この作業を他のマシンでも行う。<br>
また、hostfileで以下のように編集する。<br>
192.168.20.229 slots=1
192.168.20.201 slots=1
192.168.20.204 slots=1
192.168.20.230 slots=1

ここでは、各マシンのプロセスを1に設定した。<br>
その後、コンパイする。<br>
`mpicc hello.c -o hello`<br>
`mpicc sum.c -o sum`<br>
この後にhello worldプログラムと、並列計算用のプログラムを実行する。<br>
`mpirun --hostfile hostfile ./hello`<br>
`mpirun --hostfile hostfile ./sum`<br>


