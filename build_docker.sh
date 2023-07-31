#!/bin/bash
######## 配置默认预置参数 ########

cd `dirname $0`

registry_hk_host="1repo.corp.matrx.team"
registry_ab_host="harbor-g42c.corp.matrx.team"
registry_s5k_host="10.200.2.153:8080"
registry_bj_host="192.168.205.62"
registry_bj_ext="111.203.152.50:2443"
registry_url="/matrx-docker"
registry_host='unset'
date_=$(date "+%m_%d_%H_%M")
project=$(basename `pwd`)
branch=$(git name-rev --name-only HEAD)
commit=$(git rev-parse --short HEAD)
help='unset'
registry='ab'
tag=${date_}"-"${commit}

usage() {                                 
  echo "Usage: $0 [ -n BRANCH ] 
                  [ -r REGISTRY bj|bjext|ab|hk|s5k default ab]
                  [ -h show this message]" 1>&2 
}

####### 通过参数修改默认参数 ########
#docker login --username=admin --password=Harbor12345 https://10.200.2.153:8080

while getopts 'hb:r:' arg #选项后面的冒号表示该选项需要参数
do
    case $arg in
        b)
            branch=$OPTARG
            ;;
        r)
            registry=$OPTARG
            ;;
        h)
            help=1
            break
            ;;
        ?)
            echo "unkonw argument"
            usage
            exit 1
    ;;
    esac
done
case $registry in
    bj)
        registry_host=$registry_bj_host
        ;;
    ab)
        registry_host=$registry_ab_host
        ;;
    bjext)
        registry_host=$registry_bj_ext
        ;;
    hk)
        registry_host=$registry_hk_ext
        ;;
    s5k)
        registry_host=$registry_s5k_ext
        ;;
    *)
        echo "unknow registry name"
        usage
        exit
;;
esac
registry_host=$registry_host$registry_url
if [[ "$help" == 1 ]]
then
    usage
    exit 0
fi

######## 最终的镜像名 ########

image_name="${registry_host}/${project}/${branch}:${tag}"

echo "=================== 最终镜像名 =================="
echo "进程名:$workername"
echo "仓库名:$registry"
echo "仓库地址：${image_name}"
echo "==============================================="

######## 编译并构建容器镜像 ########

test_result() {
    if [[ $1 -ne 0 ]]; then
        exit 1
    fi
}


#echo "==============================================="
#echo "1. 准备构建,删除之前版本..."
#sudo docker rmi $(docker images | grep "srs" | awk '{print $3}')
#sudo docker rmi -f ${image_name}
#sudo docker images

#rm -rf ./bin/* ./utility/pkg/* ./pkg/*
#ls ./bin/
#ls ./utility/pkg/

#echo "==============================================="
#echo "2. 编译构建应用程序..."
#cd src
#go mod vendor
#GOOS=linux GOARCH=amd64 go build -tags netgo -i -o ../bin/${project} main.go
#bash make.sh
#test_result $?

#cd -
#pwd
echo "==============================================="
echo "1. 开始构建docker镜像..."
docker build -f Dockerfile -t ${image_name}  .
test_result $?


echo "==============================================="
echo "2. 镜像构建成功，开始上传至远程仓库"
docker push ${image_name}
test_result $?

echo "==============================================="
echo "3. 删除本地镜像"
docker rmi ${image_name}

echo "==============================================="
echo "4. 镜像构建并上传至远程仓库成功: "
echo ${image_name}

