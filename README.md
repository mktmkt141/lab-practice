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
`sudo nano /etc/exports`<br>←nfsサーバがクライアントに共有するディレクトリの一覧を記述したファイルである「/etc/exports」に設定を記述する。<br>
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
`echo "Hello from client" | sudo tee /mnt/nfs_mount/hello.txt`<br>←vm1で/mnt/nfs_mount/hello.txtに「Hello from client」と記述した<br>
**vm0の設定**<br>
`cat /home/admin/nfs_share/hello.txt`←vm1で作った内容をvm0の上で見ることが出来た<br>



