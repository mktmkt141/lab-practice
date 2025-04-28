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
vm0をldapサーバ、vm1をldapクライアントとした。
1.OpenLdapサーバの構築<br>
`sudo dnf install epel-release`←epleリポジトリ（yum等にはないパッケージををインストールするためのサードパーティーリポジトリ)のインストールを行う<br>
`sudo yum -y install openldap*`←opneldap関連のパッケージをまとめてインストールするためのコマンド<br>
`sudo slappasswd`←ldap管理者用のパスワードをを暗号化形式で発行する<br>
`sudo nano ldaproot.ldif`←LDAPの管理者アカウントをなどを設定するためのldifファイルを作成する<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f ldaproot.ldif`←ldaproot.ldifファイルをldapサーバに反映させる<br>
ここまでがldapサーバの設定内容（管理者パスワード、ドメイン名の設定を行った）<br>

ここから、ssl/tlsの設定を行う。<br>
`sudo nano /etc/ssl/openssl.conf`←証明書生成時に使用する設定ファイルを編集する<br>
`sudo openssl genrsa -aes128 -out /etc/pki/tls/certs/server.key 2048`←秘密鍵を作成し、`/etc/pki/tls/certs`に出力<br>
`sudo openssl rsa -in server.key -out server.key`←パスフレーズを除去した秘密鍵に変換する（ldapサーバ起動時にパスフレーズ入力を求められないようにするため）<br>
`sudo openssl req -utf8 -new -key server.key -out server.csr`←秘密鍵を使って、証明書署名要求（csrファイル）を作成する<br>
`sudo openssl x509 -in server.csr -out server.crt -req -signkey server.key -extfile /etc/ssl/openssl.cnf -extensions example.com -days 3650`←証明書の有効期限を10年に設定した。また、csrに対して自己署名し、サーバ証明書を作成する<br>
`sudo cp /etc/pki/tls/certs/{server.key,server.crt} /etc/openldap/certs/`←作成した秘密鍵を・証明書ldapサーバ専用のディレクトリ`/etc/openldap/certs/`に配置する<br>
`chown ldap:ldap /etc/openldap/certs/{server.key,server.crt}`←ファイルの所有者をldapユーザとグループに変更する（slapdが読み取れるようにするため）<br>
`nano mod_ssl.ldif`←ldapサーバにssl/tlsを有効化する設定を書いたldifファイルを作成する<br>
`sudo ldapmodify -Y EXTERNAL -H ldapi:/// -f mod_ssl.ldif`←mod_ssl/ldifを読み込んで、ssl/tlsを有効化する設定をサーバに反映させる<br>

`sudo slappaswd`←







