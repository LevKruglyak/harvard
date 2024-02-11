\ProvidesClass{lkx_pset}[9/16/23 v0.1 Problem Set Class]

\LoadClass[11pt,letterpaper]{article}
\usepackage[portrait, margin=0.5in, includefoot]{geometry}

% -----------------------------------------------
%                  Options 
% -----------------------------------------------

\usepackage{ifthen}
\newboolean{expanded}
\setboolean{expanded}{false}
\DeclareOption{expanded}{
	\setboolean{expanded}{true}
}
\ProcessOptions\relax

% -----------------------------------------------
%                   Title
% -----------------------------------------------

\usepackage{titling}

\let\oldmaketitle\maketitle
\renewcommand{\maketitle}{
	\ifthenelse{\boolean{expanded}}
	{
		\renewcommand\maketitlehooka{\null\mbox{}\vfill}
		\renewcommand\maketitlehookd{\vfill\null}

		\begin{titlingpage}
			\oldmaketitle
			\collaborators
		\end{titlingpage}
	}
	{
		\oldmaketitle
		\collaborators
	}
}

\makeatletter
\def \title #1 {%
	\def\@title{\textbf{#1}}%
}%
\def \due #1 {
	\def\@date{\textbf{Due:} #1}%}
}
\makeatother

\makeatletter
\def \author #1 {%
	\def\@author{#1}
}%
\makeatother

% -----------------------------------------------
%                   Collaborators
% -----------------------------------------------

\usepackage{etoolbox}

\newtoggle{firstcollaborator}
\toggletrue{firstcollaborator}
\newcommand\collablist{}

\newcommand\collaborator[1]{%
	\iftoggle{firstcollaborator}
	{%
		\def\collablist{#1}
		\togglefalse{firstcollaborator}%
	}
	{%
		\gappto\collablist{, #1}
	}
}

\newcommand\collaborators{%
	\ifboolexpr{ test {\ifdefempty{\collablist}} }
	{}
	{\begin{center}{Collaborators: \textit{\collablist}}\end{center}\medskip}
}

% -----------------------------------------------
%              Colors
% -----------------------------------------------

\usepackage[dvipsnames]{xcolor}

\definecolor{greenback}{RGB}{235, 246, 240}
\definecolor{greenbord}{RGB}{20, 138, 67}

\definecolor{orangeback}{RGB}{246, 237, 229}
\definecolor{orangebord}{RGB}{132, 46, 8}

\definecolor{blueback}{RGB}{237, 241, 248}
\definecolor{bluebord}{RGB}{11, 88, 169}

% -----------------------------------------------
%              Environments
% -----------------------------------------------

\usepackage{amsthm}
\usepackage{xparse}
\usepackage[most]{tcolorbox}

\def \theoremtitle{\fontfamily{phv}\bfseries}

\newcommand{\createcolortheorem}[3]{
	\tcolorboxenvironment{#1}{
		colback=#2, colframe=#3,
		enhanced jigsaw, sharp corners,
		leftrule=1pt, rightrule=1pt, toprule=1pt, bottomrule=1pt
		% before=\par\medskip\noindent,parbox=false
	}
}

\newtheoremstyle{oldproblem}{}{}{}{}{\theoremtitle\color{bluebord}}{}{ }{}
\theoremstyle{oldproblem}
\newtheorem{innerproblem}{Problem}
\newenvironment{oldproblem}[1]
{\renewcommand\theinnerproblem{#1.}\innerproblem}
{\endinnerproblem}
\createcolortheorem{oldproblem}{blueback}{bluebord}

\newenvironment{problem}[1]{
	\ifthenelse{\boolean{expanded}}{\newpage}{}
	\begin{oldproblem}{#1}}{\end{oldproblem}}

\newtheoremstyle{example}{}{}{}{}{\theoremtitle\color{greenbord}}{}{ }{}
\theoremstyle{example}
\newtheorem{innerexample}{Example.}
\newenvironment{example}
{\renewcommand\theinnerexample{}\innerexample}
{\endinnerexample}
\createcolortheorem{example}{greenback}{greenback}

\newtheoremstyle{definition}{}{}{}{}{\theoremtitle\color{greenbord}}{}{ }{}
\theoremstyle{definition}
\newtheorem{innerdefinition}{Definition.}
\newenvironment{definition}
{\renewcommand\theinnerdefinition{}\innerdefinition}
{\endinnerdefinition}
\createcolortheorem{definition}{greenback}{greenback}

\usepackage{xstring}
\newtheoremstyle{oldpart}{}{}{}{}{\theoremtitle\color{bluebord}}{}{0.0001pt}{}
\theoremstyle{oldpart}
\newtheorem{innerpart}{}
\newenvironment{partthm}[1]
{
	\ifthenelse{\equal{#1}{}}
	{\renewcommand\theinnerpart{\hspace{0em}}}
	{\renewcommand\theinnerpart{#1. }}
	\innerpart
}{\endinnerpart}
\createcolortheorem{partthm}{blueback}{blueback}

\renewenvironment{part}[1]
{
	\item\leavevmode\vspace{-1.5em} \begin{partthm}{#1}
		}{
	\end{partthm}
}

% -----------------------------------------------
%        Claim/Lemma/Theorem environments
% -----------------------------------------------

% Here we need to do some hacking to ensure that everything
% looks good irrespective of whether a number was provided or not

\newtheoremstyle{plain}{}{}{\em}{}{\bf}{.}{ }{}

\newtheoremstyle{claim}{}{}{}{}{\theoremtitle\color{orangebord}}{}{ }{}
\theoremstyle{claim}
\newtheorem{innerclaim}{Claim}
\newtheorem{innertheorem}{Theorem}

\newtheoremstyle{emptyclaim}{}{}{}{}{\theoremtitle\color{orangebord}}{}{ }{}
\theoremstyle{emptyclaim}
\newtheorem{emptyinnerclaim}{Claim.}
\newtheorem{emptyinnertheorem}{Theorem.}

\NewDocumentEnvironment{claim}{g}
{\IfNoValueTF{#1}
	{\renewcommand\theemptyinnerclaim{}\emptyinnerclaim}
	{\renewcommand\theinnerclaim{#1.}\innerclaim}
}
{\endinnerclaim}
\createcolortheorem{claim}{orangeback}{orangeback}

\NewDocumentEnvironment{theorem}{g}
{\IfNoValueTF{#1}
	{\renewcommand\theemptyinnertheorem{}\emptyinnertheorem}
	{\renewcommand\theinnertheorem{#1.}\innertheorem}
}
{\endinnertheorem}
\createcolortheorem{theorem}{orangeback}{orangeback}

\renewenvironment{proof}{{\theoremtitle\color{orangebord} Proof. }}{\qed}
\tcolorboxenvironment{proof} {
	before skip=0pt,
	colframe=white, colback=white,
	enhanced jigsaw, sharp corners, breakable,
	leftrule=0pt, rightrule=0pt, toprule=0pt, bottomrule=0pt
}

% -----------------------------------------------
%        Solution/Changemargins environment
% -----------------------------------------------

\newenvironment{changemargins}{%
\begin{list}{}{%
		\setlength{\itemsep}{0em}
		\setlength{\topsep}{0em}%
		\setlength{\leftmargin}{1em}%
		\setlength{\rightmargin}{1em}%
		\setlength{\listparindent}{\parindent}%
		\setlength{\itemindent}{\parindent}%
		\setlength{\parsep}{\parskip}%
	}%
	}{
\end{list}
{\medskip}
}

\newenvironment{solution}{
	\begin{changemargins}
		\item
		}{
	\end{changemargins}
}

\newenvironment{parts}{
	\begin{changemargins}
		}{
	\end{changemargins}
}

% -----------------------------------------------
%              Box environment
% -----------------------------------------------

\usepackage{collectbox}
\makeatletter
\newcommand{\boxsurround}{%
	\collectbox{%
		\setlength{\fboxsep}{1em}%
		\fbox{\BOXCONTENT}%
	}%
}
\makeatother

% -----------------------------------------------
%              Basic math stuff
% -----------------------------------------------

\usepackage{amsmath, amsfonts, mathtools, amsthm, amssymb} % Standard ams libraries
\usepackage{mathrsfs} % Fancy script capitals
\usepackage{bm} % Bold math

% -----------------------------------------------
%              Basic math/formatting stuff
% -----------------------------------------------

\providecommand{\todo}[1]{{\bf\color{red}{#1}}}

\providecommand{\Z}{\mathbb{Z}}
\providecommand{\Q}{\mathbb{Q}}
\providecommand{\R}{\mathbb{R}}
\providecommand{\N}{\mathbb{N}}
\providecommand{\C}{\mathbb{C}}
\providecommand{\CP}{\mathbb{CP}}
\providecommand{\RP}{\mathbb{RP}}
\providecommand{\F}{\mathbb{F}}
\providecommand{\A}{\mathbb{A}}
\providecommand{\Gr}{\textrm{Gr}}
\providecommand{\lcm}{\textrm{lcm}}
\providecommand{\gcd}{\textrm{gcd}}
\providecommand{\End}{\textrm{End}}
\providecommand{\Hom}{\textrm{Hom}}
\providecommand{\Ann}{\textrm{Ann}}
\providecommand{\Aut}{\textrm{Aut}}
\providecommand{\GL}{\textrm{GL}}
\providecommand{\SL}{\textrm{SL}}
\providecommand{\geq}{\geqslant}
\providecommand{\leq}{\leqslant}

% We want skips between paragraphs, but also keep indents
\edef\restoreparindent{\parindent=\the\parindent\relax}
\usepackage{parskip}
\restoreparindent

% Lists
\usepackage[shortlabels]{enumitem}
\setlist[enumerate]{topsep=1ex,itemsep=1ex,partopsep=1ex,parsep=1ex}
\setlist[itemize]{topsep=1ex,itemsep=1ex,partopsep=1ex,parsep=1ex}

% Better empty set
\usepackage{amssymb}
\let\oldemptyset\emptyset
\let\emptyset\varnothing

% Font stuff
%\usepackage{cmbright}
\usepackage{helvet}

% Large block matrices
\providecommand{\bigletter}[1]{\mbox{\normalfont\Large\bfseries {#1}}}
\providecommand{\rvline}{\hspace*{-\arraycolsep}\vline\hspace*{-\arraycolsep}}

% Floor and ceiling
\DeclarePairedDelimiter\ceil{\lceil}{\rceil}
\DeclarePairedDelimiter\floor{\lfloor}{\rfloor}
\providecommand\norm[1]{\left\lVert#1\right\rVert}

% Restriction
\newcommand\restr[2]{{% we make the whole thing an ordinary symbol
\left.\kern-\nulldelimiterspace % automatically resize the bar with \right
#1 % the function
\vphantom{\big|} % pretend it's a little taller at normal size
\right|_{#2} % this is the delimiter
}}

\newcommand\defint[3]{{% we make the whole thing an ordinary symbol
\left.\kern-\nulldelimiterspace % automatically resize the bar with \right
#1 % the function
\vphantom{\big|} % pretend it's a little taller at normal size
\right|^{#3}_{#2} % this is the delimiter
}}


\usepackage{tikz}
\usetikzlibrary{3d}
\usetikzlibrary{hobby}
\usetikzlibrary{calc}
\usetikzlibrary{patterns}
\usetikzlibrary{decorations}
\usetikzlibrary{decorations.markings}
\usetikzlibrary{decorations.text}
\usetikzlibrary{shapes,intersections}
\usepackage{tikz-cd}

% *** quiver ***
% A package for drawing commutative diagrams exported from https://q.uiver.app.
%
% This package is currently a wrapper around the `tikz-cd` package, importing necessary TikZ
% libraries, and defining a new TikZ style for curves of a fixed height.
%
% Version: 1.3.0
% Authors:
% - varkor (https://github.com/varkor)
% - AndréC (https://tex.stackexchange.com/users/138900/andr%C3%A9c)

% `tikz-cd` is necessary to draw commutative diagrams.
\RequirePackage{tikz-cd}
% `amssymb` is necessary for `\lrcorner` and `\ulcorner`.
\RequirePackage{amssymb}
% `calc` is necessary to draw curved arrows.
\usetikzlibrary{calc}
% `pathmorphing` is necessary to draw squiggly arrows.
\usetikzlibrary{decorations.pathmorphing}

% A TikZ style for curved arrows of a fixed height, due to AndréC.
\tikzset{curve/.style={settings={#1},to path={(\tikztostart)
					.. controls ($(\tikztostart)!\pv{pos}!(\tikztotarget)!\pv{height}!270:(\tikztotarget)$)
					and ($(\tikztostart)!1-\pv{pos}!(\tikztotarget)!\pv{height}!270:(\tikztotarget)$)
					.. (\tikztotarget)\tikztonodes}},
	settings/.code={\tikzset{quiver/.cd,#1}
			\def\pv##1{\pgfkeysvalueof{/tikz/quiver/##1}}},
	quiver/.cd,pos/.initial=0.35,height/.initial=0}

% TikZ arrowhead/tail styles.
\tikzset{tail reversed/.code={\pgfsetarrowsstart{tikzcd to}}}
\tikzset{2tail/.code={\pgfsetarrowsstart{Implies[reversed]}}}
\tikzset{2tail reversed/.code={\pgfsetarrowsstart{Implies}}}
% TikZ arrow styles.
\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}

\endinput
