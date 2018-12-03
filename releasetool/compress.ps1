
Remove-Item -Recurse ./MyProjectBuildScript/Plugins/VRM4U/Intermediate
Remove-Item -Recurse ./MyProjectBuildScript/Plugins/VRM4U/Binaries/Win64/*.pdb
Remove-Item -Recurse ./MyProjectBuildScript/Plugins/VRM4U/Source

Compress-Archive -Force -Path ./MyProjectBuildScript/Plugins -DestinationPath $Args[0]


