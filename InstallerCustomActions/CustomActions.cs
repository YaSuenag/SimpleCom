using System;
using System.Collections;
using System.ComponentModel;
using System.Configuration.Install;
using System.IO;

namespace SimpleComInstaller
{
    [RunInstaller(true)]
    public class CustomActions : Installer
    {

        private const string fragment_json = @"
            {
                ""profiles"": [
                    {
                        ""name"": ""SimpleCom"",
                        ""commandline"": ""{PATH}""
                    }
                ]
            }
        ";

        private string FragmentPath => Environment.GetEnvironmentVariable(this.Context.Parameters["allusers"] == "1" ? "ProgramData" : "LocalAppData")
                                            + @"\Microsoft\Windows Terminal\Fragments\SimpleCom";

        public override void Install(IDictionary stateSaver)
        {
            base.Install(stateSaver);
            var path = this.Context.Parameters["TargetDir"];
            var fragments = fragment_json.Replace("{PATH}", (path.Trim() + @"SimpleCom.exe").Replace(@"\", @"\\"));

            if (!Directory.Exists(FragmentPath))
            {
                Directory.CreateDirectory(FragmentPath);
            }
            File.WriteAllText(FragmentPath + @"\fragments.json", fragments);
        }

        public override void Rollback(IDictionary savedState)
        {
            base.Rollback(savedState);
            Directory.Delete(FragmentPath, true);
        }

        public override void Uninstall(IDictionary savedState)
        {
            base.Uninstall(savedState);
            Directory.Delete(FragmentPath, true);
        }
    }
}
