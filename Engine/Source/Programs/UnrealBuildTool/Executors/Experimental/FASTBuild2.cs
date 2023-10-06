// Original Copyright 2018 Yassine Riahi and Liam Flookes. Provided under a MIT License, see license file on github https://github.com/liamkf/Unreal_FASTBuild.
// Used to generate a fastbuild .bff file from UnrealBuildTool to allow caching and distributed builds.

// Tested with Windows 10, Visual Studio 2019 Professional, Unreal Engine 4.25.0, FastBuild v1.01.

/*
The MIT License (MIT)

Copyright (c) 2016 KnownShippable (Yassine Riahi & Liam Flookes)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;
using System.Text;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	// https://www.fastbuild.org/docs/functions/compiler.html
	internal class BffCompilier
	{
		public string CompilierName;
		public FileReference ExecutablePath;
		public HashSet<FileReference> ExtraFiles = new HashSet<FileReference>();
		public Dictionary<string, object> OtherOptions = new Dictionary<string, object>();

		public string ToBffString()
		{
			var builder = new StringBuilder();
			builder.AppendLine("Compiler('{0}')", CompilierName);
			builder.AppendLine("{");
			builder.AppendLine("\t.Executable = '{0}'", ExecutablePath.FullName);
			if (ExtraFiles.Count > 0)
			{
				builder.AppendLine("\t.ExtraFiles =");
				builder.AppendLine("\t{");
				var extraFilesList = ExtraFiles.Select(x => x.FullName).ToList();
				extraFilesList.Sort();
				foreach (var path in extraFilesList)
				{
					builder.AppendLine("\t\t'{0}',", path);
				}
				builder.AppendLine("\t}");
			}

			foreach (var option in OtherOptions)
			{
				if (option.Value.GetType() == typeof(string))
				{
					builder.AppendLine("\t{0} = '{1}'", option.Key, (string)option.Value);
				}
				else if (option.Value.GetType() == typeof(bool))
				{
					builder.AppendLine("\t{0} = {1}", option.Key, (bool)option.Value ? "true" : "false");
				}
				else if (option.Value.GetType() == typeof(int))
				{
					builder.AppendLine("\t{0} = {1}", option.Key, option.Value.ToString());
				}
				else
				{
					// Unsupported type, ignore
					Log.TraceWarning("Unsupported FastBuild Type '{0}' for Option '{1}', ignoring.", option.Value.GetType().Name, option.Value);
				}
			}

			builder.AppendLine("}");
			return builder.ToString();
		}

		public bool AddExtraFile(FileReference path)
		{
			// Don't allow adding another file with the same name as the executable
			if (ExecutablePath.GetFileName().Equals(path.GetFileName()))
			{
				return false;
			}

			// Don't allow adding a second file with the same filename
			if (ExtraFiles.Any(x => x.GetFileName().Equals(path.GetFileName())))
			{
				return false;
			}

			ExtraFiles.Add(path);
			return true;
		}
	}

	// https://www.fastbuild.org/docs/functions/settings.html
	internal class BffSettings
	{
		public DirectoryReference CachePath;
		public HashSet<string> Workers = new HashSet<string>();
		public Dictionary<string, string> Environment = new Dictionary<string, string>();

		public string ToBffString()
		{
			var builder = new StringBuilder();
			builder.AppendLine("Settings");
			builder.AppendLine("{");
			if (CachePath != null)
			{
				builder.AppendLine("\t.CachePath = '{0}'", CachePath);
			}
			if (Workers.Count > 0)
			{
				builder.AppendLine("\t.Workers =");
				builder.AppendLine("\t{");
				foreach (var worker in Workers)
				{
					builder.AppendLine("\t\t'{0}',", worker);
				}
				builder.AppendLine("\t}");
			}
			if (Environment.Count > 0)
			{
				builder.AppendLine("\t.Environment =");
				builder.AppendLine("\t{");
				foreach (var environment in Environment)
				{
					builder.AppendLine("\t\t'{0}={1}',", environment.Key, environment.Value);
				}
				builder.AppendLine("\t}");
			}

			builder.AppendLine("}");
			return builder.ToString();
		}
	}

	internal abstract class BffBuildAction
	{
		public Action SourceAction;
		public bool AllowDistribution = false;
		public abstract string ToBffString();
		public void PreBuildDependenciesString(StringBuilder builder)
		{
			if (SourceAction.PrerequisiteActions.Count > 0)
			{
				builder.AppendLine("\t.PreBuildDependencies =");
				builder.AppendLine("\t{");
				foreach (var depend in SourceAction.PrerequisiteActions)
				{
					builder.AppendLine("\t\t'{0}',", depend.StatusDescription);
				}
				builder.AppendLine("\t}");
			}
		}
		public void AllowDistributionString(StringBuilder builder)
		{
			if (!AllowDistribution)
			{
				builder.AppendLine("\t.AllowCaching = false");
				builder.AppendLine("\t.AllowDistribution = false");
			}
		}
	}

	// https://www.fastbuild.org/docs/functions/exec.html
	internal class BffExec : BffBuildAction
	{
		public override string ToBffString()
		{
			var builder = new StringBuilder();
			builder.AppendLine("; {0} {1}", SourceAction.CommandPath, SourceAction.CommandArguments);
			builder.AppendLine("Exec('{0}')", SourceAction.StatusDescription);
			builder.AppendLine("{");
			builder.AppendLine("\t.ExecExecutable = '{0}'", SourceAction.CommandPath);
			if (SourceAction.WorkingDirectory != null)
			{
				builder.AppendLine("\t.ExecWorkingDir = '{0}'", SourceAction.WorkingDirectory);
			}
			if (!string.IsNullOrEmpty(SourceAction.CommandArguments))
			{
				builder.AppendLine("\t.ExecArguments = '{0}'", SourceAction.CommandArguments);
			}
			if (SourceAction.ProducedItems.Count > 0)
			{
				builder.AppendLine("\t.ExecOutput = '{0}'", SourceAction.ProducedItems[0].FullName);
			}
			builder.AppendLine("\t.ExecAlways = true");
			PreBuildDependenciesString(builder);
			AllowDistributionString(builder);
			builder.AppendLine("}");
			return builder.ToString();
		}
	}

	// https://www.fastbuild.org/docs/functions/objectlist.html
	internal class BffObjectListCompile : BffBuildAction
	{
		public BffCompilier Compiler;
		public FileReference InputFile;
		public DirectoryReference OutputPath;
		public string Options;
		public string OutputExtension;

		public override string ToBffString()
		{
			var builder = new StringBuilder();
			builder.AppendLine("; {0} {1}", SourceAction.CommandPath, SourceAction.CommandArguments);
			builder.AppendLine("ObjectList('{0}')", SourceAction.StatusDescription);
			builder.AppendLine("{");
			builder.AppendLine("\t.Compiler = '{0}'", Compiler.CompilierName);
			builder.AppendLine("\t.CompilerInputFiles = '{0}'", InputFile.FullName);
			builder.AppendLine("\t.CompilerOutputPath = '{0}'", OutputPath.FullName);
			builder.AppendLine("\t.CompilerOptions = ' {0}'", Options);
			builder.AppendLine("\t.CompilerOutputExtension = '{0}'", OutputExtension);
			PreBuildDependenciesString(builder);
			AllowDistributionString(builder);
			builder.AppendLine("}");
			return builder.ToString();
		}
	}

	// https://www.fastbuild.org/docs/documentation.html
	internal class BffFile
	{
		public HashSet<string> Imports = new HashSet<string>();
		public BffSettings Settings = new BffSettings();
		public List<BffCompilier> Compiliers = new List<BffCompilier>();
		public List<BffBuildAction> BuildActions = new List<BffBuildAction>();

		public string ToBffString()
		{
			var builder = new StringBuilder();
			builder.AppendLine("// UnrealEditor FASTBuild configuration file");
			builder.AppendLine("// Autogenerated at {0} {1}", DateTime.Now.ToLongDateString(), DateTime.Now.ToLongTimeString());
			builder.AppendLine();
			builder.AppendLine("// Import Environment Variables");
			foreach (var import in Imports)
			{
				builder.AppendLine("#import {0}", import);
			}
			builder.AppendLine();
			builder.AppendLine(Settings.ToBffString());

			builder.AppendLine("// Custom Compilers");
			foreach (var compilier in Compiliers)
			{
				builder.AppendLine(compilier.ToBffString());
			}

			builder.AppendLine("// Build Actions");
			foreach (var action in BuildActions)
			{
				builder.AppendLine(action.ToBffString());
			}

			builder.AppendLine("// Build Alias");
			builder.AppendLine("Alias('all')");
			builder.AppendLine("{");
			builder.AppendLine("\t.Targets =");
			builder.AppendLine("\t{");
			foreach (var action in BuildActions)
			{
				builder.AppendLine("\t\t'{0}',", action.SourceAction.StatusDescription);
			}
			builder.AppendLine("\t}");
			builder.AppendLine("}");

			return builder.ToString();
		}
	}

	class FASTBuild2 : ActionExecutor
	{
		/*---- Configurable User settings ----*/

		// Used to specify a non-standard location for the FBuild.exe, for example if you have not added it to your PATH environment variable.
		private static string FBuildExePathOverride = string.Empty;

		// Controls network build distribution
		private static bool bEnableDistribution = true;

		// Controls whether to use caching at all. CachePath and CacheMode are only relevant if this is enabled.
		private static bool bEnableCaching = true;

		// Location of the shared cache, it could be a local or network path (i.e: @"\\HOSTNAME\FASTBuildCache").
		// Only relevant if bEnableCaching is true;
		// Will use path in FASTBUILD_CACHE_PATH environment variable if set
		private static string CachePath = Path.Combine(Path.GetTempPath(), "FastBuildLocalCache");

		// Override list of workers (if broker is unreachable) separated by Path.PathSeparator (; for windows)
		// Found in environment variable FASTBUILD_WORKERS
		private static List<string> OverrideWorkers = new List<string>();

		public enum eCacheMode
		{
			ReadWrite, // This machine will both read and write to the cache
			ReadOnly,  // This machine will only read from the cache, use for developer machines when you have centralized build machines
			WriteOnly, // This machine will only write from the cache, use for build machines when you have centralized build machines
		}

		// Cache access mode
		// Only relevant if bEnableCaching is true;
		// Will use value in FASTBUILD_CACHE_MODE environment variable if set
		private static eCacheMode CacheMode = eCacheMode.ReadWrite;

		// Do not modify
		private static HashSet<string> ParseSkipTokens = new HashSet<string>(new[] { "/I", "/l", "/D", "-D", "-x", "-include", "-include-pch", "-target" });

		static FASTBuild2()
		{
			var cachePath = Environment.GetEnvironmentVariable("FASTBUILD_CACHE_PATH");
			var cacheMode = Environment.GetEnvironmentVariable("FASTBUILD_CACHE_MODE");
			var workers = Environment.GetEnvironmentVariable("FASTBUILD_WORKERS");

			if (!string.IsNullOrEmpty(cachePath))
			{
				CachePath = cachePath;
			}

			if (!string.IsNullOrEmpty(cacheMode))
			{
				switch (cacheMode)
				{
					case "rw": { CacheMode = eCacheMode.ReadWrite; break; }
					case "r": { CacheMode = eCacheMode.ReadOnly; break; }
					case "w": { CacheMode = eCacheMode.WriteOnly; break; }
				}
			}

			if (!string.IsNullOrEmpty(workers))
			{
				OverrideWorkers.AddRange(workers.Split(Path.PathSeparator.ToString().ToCharArray(), StringSplitOptions.RemoveEmptyEntries));
			}
		}

		/*--------------------------------------*/

		public override string Name
		{
			get { return "FASTBuild2"; }
		}

		public static bool IsAvailable()
		{
			if (!string.IsNullOrEmpty(FBuildExePathOverride) && File.Exists(FBuildExePathOverride))
			{
				return true;
			}

			// Get the name of the FASTBuild executable.
			string fbuild = (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win32 || BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win64) ?
				"fbuild.exe" : "fbuild";

			// Search the path for it
			string PathVariable = Environment.GetEnvironmentVariable("PATH");
			foreach (string SearchPath in PathVariable.Split(Path.PathSeparator))
			{
				try
				{
					string PotentialPath = Path.Combine(SearchPath, fbuild);
					if (File.Exists(PotentialPath))
					{
						return true;
					}
				}
				catch (ArgumentException)
				{
					// PATH variable may contain illegal characters; just ignore them.
				}
			}
			return false;
		}

		private enum FBBuildType
		{
			Windows,
			XboxOne,
			Playstation4,
			Clang,
			Unsupported,
		}

		private FBBuildType BuildType = FBBuildType.Unsupported;

		private void DetectBuildType(List<Action> Actions)
		{
			foreach (Action action in Actions)
			{
				if (action.ActionType != ActionType.Compile && action.ActionType != ActionType.Link)
				{
					continue;
				}

				if (action.CommandPath.FullName.Contains("orbis"))
				{
					BuildType = FBBuildType.Playstation4;
					return;
				}
				else if (action.CommandArguments.Contains("Intermediate\\Build\\XboxOne"))
				{
					BuildType = FBBuildType.XboxOne;
					return;
				}
				else if (action.CommandPath.FullName.Contains("Microsoft") || action.CommandArguments.Contains("MSVC"))
				{
					BuildType = FBBuildType.Windows;
					return;
				}
				else if (action.CommandPath.FullName.Contains("clang++.exe"))
				{
					BuildType = FBBuildType.Clang;
					return;
				}
			}
		}

		private bool IsMSVC() { return BuildType == FBBuildType.Windows || BuildType == FBBuildType.XboxOne; }
		private bool IsPS4() { return BuildType == FBBuildType.Playstation4; }
		private bool IsClang() { return BuildType == FBBuildType.Clang; }
		private string GetCompilerName()
		{
			switch (BuildType)
			{
				default:
				case FBBuildType.XboxOne:
				case FBBuildType.Windows: return "UE4Compiler";
				case FBBuildType.Playstation4: return "UE4PS4Compiler";
				case FBBuildType.Clang: return "UE4ClangCompiler";
			}
		}

		//Run FASTBuild on the list of actions. Relies on fbuild.exe being in the path.
		public override bool ExecuteActions(List<Action> Actions, bool bLogDetailedActionStats)
		{
			DetectBuildType(Actions);
			if (BuildType == FBBuildType.Unsupported)
			{
				LocalExecutor localExecutor = new LocalExecutor();
				return localExecutor.ExecuteActions(Actions.ToList(), bLogDetailedActionStats);
			}

			bool FASTBuildResult;

			string FASTBuildFilePath = Path.Combine(UnrealBuildTool.EngineDirectory.FullName, "Intermediate", "Build", "fbuild.bff");

			if (CreateBffFile(Actions, FASTBuildFilePath))
			{
				FASTBuildResult = ExecuteBffFile(FASTBuildFilePath);

				string ReportFilePath = Path.Combine(UnrealBuildTool.EngineDirectory.MakeRelativeTo(DirectoryReference.GetCurrentDirectory()), "Source", "report.html");
				GenerateDependencyFiles(Actions, ReportFilePath);
			}
			else
			{
				Log.TraceWarning("Failed creating FastBuild .bff file. Executing actions using LocalExecutor.");
				LocalExecutor localExecutor = new LocalExecutor();
				FASTBuildResult = localExecutor.ExecuteActions(Actions, bLogDetailedActionStats);
			}

			return FASTBuildResult;
		}

		private string SubstituteEnvironmentVariables(string commandLineString)
		{
			string outputString = commandLineString.Replace("$(DurangoXDK)", "$DurangoXDK$");
			outputString = outputString.Replace("$(SCE_ORBIS_SDK_DIR)", "$SCE_ORBIS_SDK_DIR$");
			outputString = outputString.Replace("$(DXSDK_DIR)", "$DXSDK_DIR$");
			outputString = outputString.Replace("$(CommonProgramFiles)", "$CommonProgramFiles$");

			return outputString;
		}

		private void ParseOriginalCommandPathAndArguments(Action Action, ref string CommandPath, ref string CommandArguments)
		{
			if (Action.CommandPath.FullName.EndsWith("cl-filter.exe"))
			{
				// In case of using cl-filter.exe we have to extract original command path and arguments
				// Command line has format {Action.DependencyListFile.Location} -- {Action.CommandPath} {Action.CommandArguments} /showIncludes
				int SeparatorIndex = Action.CommandArguments.IndexOf(" -- ");
				string CommandPathAndArguments = Action.CommandArguments.Substring(SeparatorIndex + 4).Replace("/showIncludes", "");
				List<string> Tokens = CommandLineParser.Parse(CommandPathAndArguments);
				CommandPath = Tokens[0];
				CommandArguments = string.Join(" ", Tokens.GetRange(1, Tokens.Count - 1));
			}
			else
			{
				CommandPath = Action.CommandPath.FullName;
				CommandArguments = Action.CommandArguments;
			}
		}

		private Dictionary<string, string> ParseCommandArguments(string CommandArguments, string[] SpecialOptions, bool SkipInputFile = false)
		{
			CommandArguments = SubstituteEnvironmentVariables(CommandArguments);
			List<string> Tokens = CommandLineParser.Parse(CommandArguments);
			Dictionary<string, string> ParsedCompilerOptions = new Dictionary<string, string>();

			// Replace response file with its content
			for (int i = 0; i < Tokens.Count; i++)
			{
				if (!Tokens[i].StartsWith("@\""))
				{
					continue;
				}

				string ResponseFilePath = Tokens[i].Substring(2, Tokens[i].Length - 3);
				string ResponseFileText = SubstituteEnvironmentVariables(File.ReadAllText(ResponseFilePath));

				Tokens.RemoveAt(i);
				Tokens.InsertRange(i, CommandLineParser.Parse(ResponseFileText));

				if (ParsedCompilerOptions.ContainsKey("@"))
				{
					throw new Exception("Only one response file expected");
				}

				ParsedCompilerOptions["@"] = ResponseFilePath;
			}

			// Search tokens for special options
			foreach (string SpecialOption in SpecialOptions)
			{
				for (int i = 0; i < Tokens.Count; ++i)
				{
					if (Tokens[i] == SpecialOption && i + 1 < Tokens.Count)
					{
						ParsedCompilerOptions[SpecialOption] = Tokens[i + 1];
						Tokens.RemoveRange(i, 2);
						break;
					}
					else if (Tokens[i].StartsWith(SpecialOption))
					{
						ParsedCompilerOptions[SpecialOption] = Tokens[i].Replace(SpecialOption, null);
						Tokens.RemoveAt(i);
						break;
					}
				}
			}

			//The search for the input file... we take the first non-argument we can find
			if (!SkipInputFile)
			{
				for (int i = 0; i < Tokens.Count; ++i)
				{
					string Token = Tokens[i];
					// Skip tokens with values, I for cpp includes, l for resource compiler includes
					if (ParseSkipTokens.Contains(Token))
					{
						++i;
					}
					else if (!Token.StartsWith("/") && !Token.StartsWith("-") && !Token.StartsWith("\"-"))
					{
						ParsedCompilerOptions["InputFile"] = Token;
						Tokens.RemoveAt(i);
						break;
					}
				}
			}

			ParsedCompilerOptions["OtherOptions"] = string.Join(" ", Tokens) + " ";

			return ParsedCompilerOptions;
		}

		private string GetOptionValue(Dictionary<string, string> OptionsDictionary, string Key, Action Action, bool ProblemIfNotFound = false)
		{
			string Value;
			if (OptionsDictionary.TryGetValue(Key, out Value))
			{
				return Value.Trim('\"');
			}

			if (ProblemIfNotFound)
			{
				Log.TraceError("We failed to find " + Key + ", which may be a problem.");
				Log.TraceError("Action.CommandArguments: " + Action.CommandArguments);
			}

			return string.Empty;
		}

		private void BuildEnvironmentSetup(List<Action> Actions, BffFile bffFile)
		{
			// Copy environment into a case-insensitive dictionary for easier key lookups
			Dictionary<string, string> envVars = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
			foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
			{
				envVars[(string)entry.Key] = (string)entry.Value;
			}

			foreach (var variable in new string[] { "CommonProgramFiles", "DXSDK_DIR", "DurangoXDK", "SCE_ORBIS_SDK_DIR" })
			{
				if (envVars.ContainsKey(variable))
				{
					bffFile.Imports.Add(variable);
				}
			}


			BffSettings settings = new BffSettings();
			// Optional cachePath user setting
			if (bEnableCaching && !string.IsNullOrEmpty(CachePath))
			{
				settings.CachePath = DirectoryReference.FromString(CachePath);
			}

			if (OverrideWorkers.Count > 0)
			{
				OverrideWorkers.ForEach(x => settings.Workers.Add(x));
			}

			foreach (var variable in new string[] { "TMP", "TEMP", "INCLUDE", "LIB", "SystemRoot", "CommonProgramFiles", "DXSDK_DIR", "DurangoXDK", "SCE_ORBIS_SDK_DIR", "LINUX_MULTIARCH_ROOT" })
			{
				if (envVars.ContainsKey(variable))
				{
					settings.Environment.Add(variable, envVars[variable]);
				}
			}

			if (BuildType == FBBuildType.Windows)
			{
				VCEnvironment VCEnv = VCEnvironment.Create(WindowsPlatform.GetDefaultCompiler(null), UnrealTargetPlatform.Win64, WindowsArchitecture.x64, null, null, null);

				// PATH
				List<string> systemPath = new List<string>();

				if (VCEnv.Architecture == WindowsArchitecture.x64)
				{
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x64).FullName);
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x86).FullName);
				}
				else if (VCEnv.Architecture == WindowsArchitecture.x86)
				{
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x86).FullName);
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x64).FullName);
				}
				else if (VCEnv.Architecture == WindowsArchitecture.ARM64)
				{
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.ARM64).FullName);
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x86).FullName);
					systemPath.Add(VCEnvironment.GetVCToolPath(VCEnv.ToolChain, VCEnv.ToolChainDir, WindowsArchitecture.x64).FullName);
				}

				if (VCEnv.WindowsSdkVersion >= new VersionNumber(10))
				{
					systemPath.Add(DirectoryReference.Combine(VCEnv.WindowsSdkDir, "bin", VCEnv.WindowsSdkVersion.ToString(), VCEnv.Architecture.ToString()).FullName);
				}

				if (systemPath.Count > 0)
				{
					settings.Environment.Add("PATH", string.Join(Path.PathSeparator.ToString(), systemPath));
				}

				// UE4ResourceCompiler
				FileReference resourceCompilierPath = VCEnv.ResourceCompilerPath;

				if (resourceCompilierPath != null && File.Exists(resourceCompilierPath.FullName))
				{
					BffCompilier resourceCompiler = new BffCompilier();
					resourceCompiler.CompilierName = "UE4ResourceCompiler";
					resourceCompiler.ExecutablePath = resourceCompilierPath;
					resourceCompiler.OtherOptions.Add(".CompilerFamily", "custom");
					bffFile.Compiliers.Add(resourceCompiler);
				}

				// UE4Compiler
				FileReference compilerPath = VCEnv.CompilerPath;

				if (compilerPath != null && File.Exists(compilerPath.FullName))
				{
					BffCompilier windowsCompilier = new BffCompilier();
					windowsCompilier.CompilierName = "UE4Compiler";
					windowsCompilier.ExecutablePath = compilerPath;
					windowsCompilier.OtherOptions.Add(".UseLightCache_Experimental", true);

					foreach (var path in systemPath)
					{
						foreach (var file in Directory.GetFiles(path, "*.dll*", SearchOption.AllDirectories))
						{
							windowsCompilier.AddExtraFile(FileReference.FromString(file));
						}
					}

					bffFile.Compiliers.Add(windowsCompilier);
				}

			}
			else if (BuildType == FBBuildType.XboxOne)
			{
				if (!envVars.ContainsKey("DurangoXDK"))
				{
					throw new BuildException("DurangoXDK not set when building XboxOne");
				}

				throw new BuildException("XboxOne not currently supported");
			}
			else if (BuildType == FBBuildType.Playstation4)
			{
				if (envVars.ContainsKey("SCE_ORBIS_SDK_DIR"))
				{
					DirectoryReference binDirectory = DirectoryReference.FromString(Path.Combine(envVars["SCE_ORBIS_SDK_DIR"], "host_tools", "bin"));
					BffCompilier ps4Compilier = new BffCompilier();
					ps4Compilier.CompilierName = "UE4PS4Compiler";
					ps4Compilier.ExecutablePath = FileReference.Combine(binDirectory, "orbis-clang.exe");
					ps4Compilier.AddExtraFile(FileReference.Combine(binDirectory, "orbis-snarl.exe"));
					ps4Compilier.OtherOptions.Add(".SCE_ORBIS_SDK_DIR", envVars["SCE_ORBIS_SDK_DIR"]);

					bffFile.Compiliers.Add(ps4Compilier);
				}
				else
				{
					throw new BuildException("SCE_ORBIS_SDK_DIR not set when building PS4");
				}
			}
			else if (BuildType == FBBuildType.Clang)
			{
				var clangAction = Actions.Find(x => x.CommandPath.GetFileName().Equals("clang++.exe"));

				BffCompilier clangCompilier = new BffCompilier();
				clangCompilier.CompilierName = "UE4ClangCompiler";
				clangCompilier.ExecutablePath = clangAction.CommandPath;
				clangCompilier.OtherOptions.Add(".ClangRewriteIncludes", false);
				//clangCompilier.OtherOptions.Add(".ClangFixupUnity_Disable", true);

				HashSet<DirectoryReference> directories = new HashSet<DirectoryReference>();
				directories.Add(clangAction.CommandPath.Directory);
				directories.Add(DirectoryReference.Combine(clangAction.CommandPath.Directory.ParentDirectory, "lib64"));
				directories.Add(DirectoryReference.Combine(clangAction.CommandPath.Directory.ParentDirectory, "lib"));
				foreach (var directory in directories.Where(x => Directory.Exists(x.FullName)))
				{
					foreach (var file in Directory.GetFiles(directory.FullName, "*.dll", SearchOption.AllDirectories))
					{
						clangCompilier.AddExtraFile(FileReference.FromString(file));
					}
				}

				bffFile.Compiliers.Add(clangCompilier);
			}
			else
			{
				throw new BuildException("Unable to determine build platform.");
			}

			bffFile.Settings = settings;
		}

		private bool AddCompileAction(Action Action, BffFile bffFile)
		{
			string CompilerName = GetCompilerName();
			if (Action.CommandPath.FullName.Contains("rc.exe"))
			{
				CompilerName = "UE4ResourceCompiler";
			}
			string OriginalCommandPath = null;
			string OriginalCommandArguments = null;
			ParseOriginalCommandPathAndArguments(Action, ref OriginalCommandPath, ref OriginalCommandArguments);

			string[] SpecialCompilerOptions = { "/Fo", "/fo", "/Yc", "/Yu", "/Fp", "-o", "-h", "-MD", "-MF" };
			var ParsedCompilerOptions = ParseCommandArguments(OriginalCommandArguments, SpecialCompilerOptions);

			string OutputObjectFileName;
			if (IsMSVC())
			{
				OutputObjectFileName = GetOptionValue(ParsedCompilerOptions, "/Fo", Action, ProblemIfNotFound: !IsMSVC());
				if (string.IsNullOrEmpty(OutputObjectFileName))
				{
					OutputObjectFileName = GetOptionValue(ParsedCompilerOptions, "/fo", Action, ProblemIfNotFound: true);
				}
			}
			else
			{
				OutputObjectFileName = GetOptionValue(ParsedCompilerOptions, "-o", Action, ProblemIfNotFound: false);
			}


			if (string.IsNullOrEmpty(OutputObjectFileName)) //No /Fo or /fo, we're probably in trouble.
			{
				Log.TraceError("We have no OutputObjectFileName. Bailing.");
				return false;
			}

			string IntermediatePath = Path.GetDirectoryName(OutputObjectFileName);
			if (string.IsNullOrEmpty(IntermediatePath))
			{
				Log.TraceError("We have no IntermediatePath. Bailing.");
				Log.TraceError("Our Action.CommandArguments were: " + Action.CommandArguments);
				return false;
			}

			string InputFile = GetOptionValue(ParsedCompilerOptions, "InputFile", Action, ProblemIfNotFound: true);
			if (string.IsNullOrEmpty(InputFile))
			{
				Log.TraceError("We have no InputFile. Bailing.");
				return false;
			}

			BffObjectListCompile compile = new BffObjectListCompile();
			BffCompilier compiler = bffFile.Compiliers.Find(x => x.CompilierName.Equals(CompilerName));
			if (compiler == null)
			{
				return false;
			}
			compile.SourceAction = Action;
			compile.Compiler = bffFile.Compiliers.Find(x => x.CompilierName.Equals(CompilerName));
			compile.InputFile = FileReference.FromString(InputFile);
			compile.OutputPath = DirectoryReference.FromString(IntermediatePath);
			compile.AllowDistribution = Action.bCanExecuteRemotely;

			string OtherCompilerOptions = GetOptionValue(ParsedCompilerOptions, "OtherOptions", Action);
			OtherCompilerOptions = OtherCompilerOptions.Replace("we4668", "wd4668 /wd4800 /wd4459 /wd5038"); //hack for some errors with headers and stuff and complier issues that started happening in VS 16.5
			OtherCompilerOptions = OtherCompilerOptions.Replace("-Wno-undefined-bool-conversion", "-Wno-undefined-bool-conversion -Wno-unused-value -Wno-constant-logical-operand -Wno-parentheses-equality -Wno-logical-op-parentheses -Wno-deprecated-register");

			if (ParsedCompilerOptions.ContainsKey("/Yc")) // Create PCH
			{
				Log.TraceError("Create PCH Action should be run with FASTBuild Exec. Bailing.");
				return false;
			}
			else if (ParsedCompilerOptions.ContainsKey("/Yu")) // Use PCH
			{
				string PCHIncludeHeader = GetOptionValue(ParsedCompilerOptions, "/Yu", Action, ProblemIfNotFound: true);
				string PCHOutputFile = GetOptionValue(ParsedCompilerOptions, "/Fp", Action, ProblemIfNotFound: true);
				string PCHToForceInclude = PCHOutputFile.Replace(".pch", "");
				compile.Options = string.Format("/Fo\"%2\" \"%1\" /Fp\"{0}\" /Yu\"{1}\" /FI\"{2}\" {3}", PCHOutputFile, PCHIncludeHeader, PCHToForceInclude, OtherCompilerOptions);
				compile.OutputExtension = Path.GetExtension(InputFile) + ".obj";
			}
			else if (CompilerName == "UE4ResourceCompiler")
			{
				compile.Options = string.Format("{0} /Fo\"%2\" \"%1\"", OtherCompilerOptions);
				compile.OutputExtension = Path.GetExtension(InputFile) + ".res";
			}
			else if (IsMSVC())
			{
				compile.Options = string.Format("{0} /Fo\"%2\" \"%1\"", OtherCompilerOptions);
				compile.OutputExtension = Path.GetExtension(InputFile) + ".obj";
			}
			else
			{
				compile.Options = string.Format("{0} -o \"%2\" \"%1\"", OtherCompilerOptions);
				compile.OutputExtension = Path.GetExtension(Path.GetFileNameWithoutExtension(OutputObjectFileName)) + ".o";
			}

			if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win32 || BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win64)
			{
				if (compiler.ExecutablePath.FullName.Length + compile.Options.Length + compile.InputFile.FullName.Length + compile.OutputPath.FullName.Length + compile.OutputExtension.Length >= short.MaxValue)
				{
					// Args too long, use local exec instead.
					return false;
				}
			}
			bffFile.BuildActions.Add(compile);
			return true;
		}

		private bool AddLocalAction(Action Action, BffFile bffFile)
		{
			BffExec exec = new BffExec();
			exec.SourceAction = Action;
			bffFile.BuildActions.Add(exec);
			return true;
		}

		private bool IsSupportedAction(Action Action)
		{
			return Action.ActionType == ActionType.Compile && Action.bCanExecuteRemotely;
		}

		private int SortActionFunc(BffBuildAction a, BffBuildAction b)
		{
			if (a.SourceAction.PrerequisiteActions.Contains(b.SourceAction))
			{
				return 1;
			}
			else if (b.SourceAction.PrerequisiteActions.Contains(a.SourceAction))
			{
				return -1;
			}
			return Action.Compare(a.SourceAction, b.SourceAction);
		}

		private bool CreateBffFile(List<Action> InActions, string BffFilePath)
		{
			try
			{
				var bffFile = new BffFile();

				// Fix up duplicate StatusDescription by appending hash code of Command Path + Command Arguments
				foreach (var Group in InActions.GroupBy(x => x.StatusDescription).Where(g => g.Count() > 1))
				{
					foreach (var Action in Group)
					{
						var hashcode = string.Format("{0} {1}", Action.CommandPath, Action.CommandArguments).GetHashCode();
						Action.StatusDescription = string.Format("{0} (0x{1})", Action.StatusDescription, hashcode.ToString("x8"));
					}
				}
				BuildEnvironmentSetup(InActions, bffFile); // Compiler, environment variables and base paths

				foreach (var Action in InActions)
				{
					if (!IsSupportedAction(Action))
					{
						AddLocalAction(Action, bffFile);
					}
					else if (Action.ActionType == ActionType.Compile)
					{
						if (!AddCompileAction(Action, bffFile))
						{
							// Error creating action, fallback to local exec
							Action.bCanExecuteRemotely = false;
							AddLocalAction(Action, bffFile);
						}
					}
				}

				// Sort list so actions are in correct order for fastbuild
				bffFile.BuildActions.Sort((a, b) => SortActionFunc(a, b));

				using (var bffOutputFileStream = new FileStream(BffFilePath, FileMode.Create, FileAccess.Write))
				{
					byte[] data = new System.Text.UTF8Encoding(false).GetBytes(bffFile.ToBffString());
					bffOutputFileStream.Write(data, 0, data.Length);
				}
			}
			catch (Exception e)
			{
				Log.TraceError("Exception while creating bff file: " + e.ToString());
				return false;
			}

			return true;
		}

		private bool ExecuteBffFile(string BffFilePath)
		{
			string cacheArgument = "";

			if (bEnableCaching)
			{
				switch (CacheMode)
				{
					case eCacheMode.ReadOnly:
						cacheArgument = "-cacheread";
						break;
					case eCacheMode.WriteOnly:
						cacheArgument = "-cachewrite";
						break;
					case eCacheMode.ReadWrite:
						cacheArgument = "-cache";
						break;
				}
			}

			string distArgument = bEnableDistribution ? "-dist" : "";

			//Interesting flags for FASTBuild: -nostoponerror, -verbose, -monitor (if FASTBuild Monitor Visual Studio Extension is installed!)
			// Yassine: The -clean is to bypass the FastBuild internal dependencies checks (cached in the fdb) as it could create some conflicts with UBT.
			//			Basically we want FB to stupidly compile what UBT tells it to.
			string FBCommandLine = string.Format("-monitor -nostoponerror -report -summary {0} {1} -ide -wrapper -clean -config {2}", distArgument, cacheArgument, BffFilePath);

			ProcessStartInfo FBStartInfo = new ProcessStartInfo(string.IsNullOrEmpty(FBuildExePathOverride) ? "fbuild" : FBuildExePathOverride, FBCommandLine);

			FBStartInfo.UseShellExecute = false;
			FBStartInfo.WorkingDirectory = Path.Combine(UnrealBuildTool.EngineDirectory.MakeRelativeTo(DirectoryReference.GetCurrentDirectory()), "Source");

			string ReportFilePath = Path.Combine(FBStartInfo.WorkingDirectory, "report.html");
			if (File.Exists(ReportFilePath))
			{
				File.Delete(ReportFilePath);
			}

			try
			{
				Process FBProcess = new Process();
				FBProcess.StartInfo = FBStartInfo;

				FBStartInfo.RedirectStandardError = true;
				FBStartInfo.RedirectStandardOutput = true;
				FBProcess.EnableRaisingEvents = true;

				DataReceivedEventHandler OutputEventHandler = (Sender, Args) =>
				{
					if (Args.Data != null)
					{
						Console.WriteLine(Args.Data);
					}
				};

				DataReceivedEventHandler ErrorEventHandler = (Sender, Args) =>
				{
					if (Args.Data != null)
					{
						Console.Error.WriteLine(Args.Data);
					}
				};

				FBProcess.OutputDataReceived += OutputEventHandler;
				FBProcess.ErrorDataReceived += ErrorEventHandler;

				FBProcess.Start();

				FBProcess.BeginOutputReadLine();
				FBProcess.BeginErrorReadLine();

				FBProcess.WaitForExit();
				return FBProcess.ExitCode == 0;
			}
			catch (Exception e)
			{
				Log.TraceError("Exception launching fbuild process. Is it in your path?" + e.ToString());
				return false;
			}
		}

		private void GenerateDependencyFiles(List<Action> Actions, string ReportFilePath)
		{
			if (!File.Exists(ReportFilePath))
			{
				Log.TraceWarning("FASTBuild report.html not found, skipping dependency files generation");
				return;
			}

			string ReportFileText = File.ReadAllText(ReportFilePath);
			int Cursor = 0;
			while (true)
			{
				int HeaderBegin = ReportFileText.IndexOf("<h3>", Cursor);
				if (HeaderBegin == -1)
				{
					break;
				}

				int HeaderEnd = ReportFileText.IndexOf("</h3>", HeaderBegin);

				int NextHeader = ReportFileText.IndexOf("<h3>", HeaderEnd);
				if (NextHeader == -1)
				{
					NextHeader = ReportFileText.Length;
				}

				string ActionHeader = ReportFileText.Substring(HeaderBegin + 4, HeaderEnd - HeaderBegin - 4);
				Action SourceAction = Actions.Find(x => x.StatusDescription == ActionHeader);

				if (SourceAction.DependencyListFile == null || !IsSupportedAction(SourceAction))
				{
					Cursor = NextHeader;
					continue;
				}

				Cursor = HeaderEnd;

				List<string> Includes = new List<string>();
				while (true)
				{
					int IncludeEnd = ReportFileText.IndexOf("</td></tr>", Cursor);
					if (IncludeEnd == -1 || IncludeEnd > NextHeader)
					{
						break;
					}

					int IncludeBegin = ReportFileText.LastIndexOf("</td><td>", IncludeEnd);
					Cursor = IncludeEnd + 10;

					string Include = ReportFileText.Substring(IncludeBegin + 9, IncludeEnd - IncludeBegin - 9);
					if (!Include.Contains("Microsoft Visual Studio") && !Include.Contains("Windows Kits"))
					{
						Includes.Add(Include);
					}
				}

				if (SourceAction.DependencyListFile.HasExtension(".d"))
				{
					// Manually write out dependency .d file
					using (var file = new StreamWriter(SourceAction.DependencyListFile.AbsolutePath))
					{
						// Normalize slashes to forward slash
						// Escape spaces
						string normalizedPath = SourceAction.ProducedItems[0].AbsolutePath.Replace("\\", "/").Replace(" ", "\\ ");
						file.Write(normalizedPath);
						file.Write(':');
						foreach (var include in Includes)
						{
							normalizedPath = include.Replace("\\", "/").Replace(" ", "\\ ");
							file.Write(" \\\n  ");
							file.Write(normalizedPath);
						}
						file.Write('\n');
					}
				}
				else
				{
					File.WriteAllLines(SourceAction.DependencyListFile.AbsolutePath, Includes);
				}
			}
		}
	}

	internal class CommandLineParser
	{
		enum ParserState
		{
			OutsideToken,
			InsideToken,
			InsideTokenQuotes,
		}

		public static List<string> Parse(string CommandLine)
		{
			List<string> Tokens = new List<string>();

			ParserState State = ParserState.OutsideToken;
			int Cursor = 0;
			int TokenStartPos = 0;
			while (Cursor < CommandLine.Length)
			{
				char c = CommandLine[Cursor];
				if (State == ParserState.OutsideToken)
				{
					if (c == ' ' || c == '\r' || c == '\n')
					{
						Cursor++;
					}
					else
					{
						TokenStartPos = Cursor;
						State = ParserState.InsideToken;
					}
				}
				else if (State == ParserState.InsideToken)
				{
					if (c == '\\')
					{
						Cursor += 2;
					}
					else if (c == '"')
					{
						State = ParserState.InsideTokenQuotes;
						Cursor++;
					}
					else if (c == ' ' || c == '\r' || c == '\n')
					{
						Tokens.Add(CommandLine.Substring(TokenStartPos, Cursor - TokenStartPos));
						State = ParserState.OutsideToken;
					}
					else
					{
						Cursor++;
					}
				}
				else if (State == ParserState.InsideTokenQuotes)
				{
					if (c == '\\')
					{
						Cursor++;
					}
					else if (c == '"')
					{
						State = ParserState.InsideToken;
					}

					Cursor++;
				}
				else
				{
					throw new NotImplementedException();
				}
			}

			if (State == ParserState.InsideTokenQuotes)
			{
				throw new Exception("Failed to parse command line, no closing quotes found: " + CommandLine);
			}

			if (State == ParserState.InsideToken)
			{
				Tokens.Add(CommandLine.Substring(TokenStartPos));
			}

			return Tokens;
		}
	}
}