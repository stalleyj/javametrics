echo exporting JAVA_SDK_INCLUDE
export JAVA_SDK_INCLUDE=/home/alfonso/ibm-java-x86_64-71/include/

echo building 64 bit agent
make BUILD=xa64 clean javainstall

echo removing previous agent
rm /home/alfonso/ibm-java-x86_64-71/jre/bin/libhealthcenter.so

echo copying fresh agent
cp /media/sf_workspaces/healthcenter/runtime.tools/output/deploy/libhealthcenter.so /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/libhealthcenter.so

echo building common plugins
make BUILD=xa64 clean coreinstall

echo removing old common plugins, copying the fresh ones 
rm /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libenvplugin.so
rm /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libcpuplugin.so
rm /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libmemoryplugin.so
rm /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libhcmqtt.so

cp /media/sf_workspaces/healthcenter/runtime.tools/output/deploy/plugins/libenvplugin.so /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libenvplugin.so
cp /media/sf_workspaces/healthcenter/runtime.tools/output/deploy/plugins/libmemoryplugin.so /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libmemoryplugin.so
cp /media/sf_workspaces/healthcenter/runtime.tools/output/deploy/plugins/libcpuplugin.so /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libcpuplugin.so
cp /media/sf_workspaces/healthcenter/runtime.tools/output/deploy/plugins/libhcmqtt.so /home/alfonso/ibm-java-x86_64-71/jre/lib/amd64/healthcenter/libhcmqtt.so
   