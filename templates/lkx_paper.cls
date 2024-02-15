\ProvidesClass{lkx_paper}[Paper class]

\LoadClass[11pt,letterpaper]{article}
\usepackage[letterpaper, portrait, margin=1.25in, includefoot]{geometry}

\providecommand{\R}{\mathbb{R}}
\providecommand{\C}{\mathbb{C}}
\providecommand{\Z}{\mathbb{Z}}
\providecommand{\RP}{\mathbb{RP}}
\providecommand{\Hom}{\mathrm{Hom}}

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
%              Environments
% -----------------------------------------------

\newcommand\defn[1]{\textbf{#1}}
\newcommand\todo[1]{{\color{red}\textbf{#1}}}

\theoremstyle{definition}
\newtheorem{definition}{Definition}[subsection]
\newtheorem{theorem}[definition]{Theorem}
\newtheorem{remark}[definition]{Remark}
\newtheorem{proposition}[definition]{Proposition}
\newtheorem{claim}[definition]{Claim}
\newtheorem{lemma}[definition]{Lemma}
\newtheorem{example}[definition]{Example}
\newtheorem{corollary}[definition]{Corollary}

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
